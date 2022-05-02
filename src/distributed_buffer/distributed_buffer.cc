#include "distributed_buffer.h"

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <vector>
#include <utility>
#include <fstream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using graph::VertexPartition;
using graph::InteractionEdges;
using graph::Edge;
using graph::Vertex;
using graph::PartitionService;
using graph::PartitionRequest;
using graph::PartitionResponse;
using graph::PingRequest;
using graph::PingResponse;
#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif
using namespace std;

// ############################ Interaction Processing ########################
void DistributedBuffer::MarkInteraction(WorkUnit interaction) {
  // 1. Mark interaction as done
  int src_partition_id = interaction.src()->partition_id(), dst_partition_id = interaction.dst()->partition_id();
  interactions_matrix_[src_partition_id][dst_partition_id] = true;
  std::cout << "Interaction done: (" << src_partition_id << ", " << dst_partition_id << ")" << std::endl;

  // 2. Identify the outgoing partition and superpartition and check if it can be released.
  int outgoing_super_partition = partition_to_be_sent_[current_round_][self_rank_];
  int stable_super_partition = GetStablePartitionId(current_round_);
  
  if(current_round_ != partition_to_be_sent_.size() - 1) {
    // 3. Check and try to release partition if it is part of outgoing super partition.
    CheckAndReleaseOutgoingPartition(outgoing_super_partition, stable_super_partition, src_partition_id);
    if(src_partition_id != dst_partition_id) {
      CheckAndReleaseOutgoingPartition(outgoing_super_partition, stable_super_partition, dst_partition_id);
    }
  }

  // Advance round if all interactions are done for this round
  if(IsRoundComplete()) {
    std::cout << "Round " << current_round_ << " complete" << std::endl;
    current_round_++; // TODO: Might have to put a lock around it as it's read in a different thread as well
  }
}

void DistributedBuffer::FillPartitions() {
  // Epoch done when all planned partitions are fetched. -> currently terminate thread
  while(fill_round_ < plan_.size()) {
    // sleep if the buffer is full
    while(buffer_size_ == capacity_){
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Trying to request partition. Buffer size: " << buffer_size_ << std::endl;

    // Get partition according to plan
    int target_machine = plan_[fill_round_][self_rank_].first, target_super_partition = plan_[fill_round_][self_rank_].second;
    std::cout << "Fill Round " << fill_round_ << " requesting partition from " << target_machine << " for super partition " << target_super_partition << std::endl;
    // Target super partition maybe present in the buffer due to the round number not getting updated
    grpc::ClientContext context;
    PartitionRequest request;
    request.set_super_partition_id(target_super_partition);
    PartitionResponse response;

    Status status = client_stubs_[target_machine]->GetPartition(&context, request, &response);
    if (!status.ok()) {
      std::cout << "GetPartition failed: " << status.error_code() << ": " << status.error_message() << std::endl;
      continue; // Try again if failed, don't return
    }
    std::cout << "GetPartition: super_partition_id: " << response.super_partition_id() << std::endl;
    // Add partition to buffer
    graph::VertexPartition* partition = new graph::VertexPartition();
    *partition = response.partition();
    
    // Add partition to buffer and interactions corresponding to the new partition
    AddPartitionToBuffer(partition);
    AddInteractions(partition);

    // Fetch capacity_/2 partitions from other machines in every round
    partitions_fetched_[fill_round_]++;
    if(partitions_fetched_[fill_round_] == (capacity_/2)) {
      fill_round_++;
      partitions_fetched_[fill_round_] = 0;
    }
  }
}

void DistributedBuffer::AddInteractions(graph::VertexPartition* partition) {
  int stable_super_partition_id = GetStablePartitionId(fill_round_ - 1);
  int incoming_partition_id = partition->partition_id();
  // cout << "Stable super partition id: " << stable_super_partition_id << ", incoming partition id: " << incoming_partition_id << endl;
  int partition_start = stable_super_partition_id * (capacity_ /2), partition_end = (stable_super_partition_id + 1) * (capacity_ /2);

  // Determine which half is the stable super partition
  int start_idx = 0;
  if(vertex_partitions_[start_idx] != nullptr && vertex_partitions_[start_idx]->partition_id()  >= partition_start && vertex_partitions_[start_idx]->partition_id() < partition_end) {}
  else {
    start_idx = capacity_/2;
  }
  
  std::unique_lock<std::mutex> lock(mutex_);
  int added_interactions = 0;
  for(int i = start_idx; i < start_idx + capacity_/2; i++) {
    graph::VertexPartition* vp = vertex_partitions_[i];
    if(!interactions_matrix_[incoming_partition_id][vp->partition_id()]) {
      WorkUnit interaction(partition, vp, &interaction_edges_[incoming_partition_id][vp->partition_id()]);
      interaction_queue_.push(interaction);
      added_interactions++;
      // cout << "Inserted interaction: (" << incoming_partition_id << ", " << vp->partition_id() << ")" << endl;
    }
    if(!interactions_matrix_[vp->partition_id()][incoming_partition_id]) {
      WorkUnit interaction(vp, partition, &interaction_edges_[vp->partition_id()][incoming_partition_id]);
      interaction_queue_.push(interaction);
      added_interactions++;
      // cout << "Inserted interaction: (" << vp->partition_id() << ", " << incoming_partition_id << ")" << endl;
    }
  }
  std::cout << "Added " << added_interactions << " interactions" << endl;

  cv_.notify_one();
}


// @TODO: change shared ptr for interaction to unique ptr.
std::optional<WorkUnit> DistributedBuffer::GetWorkUnit() {
  std::unique_lock<std::mutex> lock(mutex_);
  if(interaction_queue_.empty() && !IsEpochComplete()) {
    std::cout << "Epoch not complete yet, but no workunits available. Waiting on CV till queue populated" << std::endl;
    cv_.wait(lock); // release lock and wait till queue populated
  }

  else if(IsEpochComplete()) {
    std::cout << "Epoch complete, returning empty optional" << std::endl;
    return std::nullopt;
  }

  auto interaction = interaction_queue_.front();
  std::cout << "Round " << current_round_ << ": " << "GetWorkUnit: (" << interaction.src()->partition_id() << ", " << interaction.dst()->partition_id() << ")" << std::endl;
  interaction_queue_.pop();
  return interaction;
}

// Determine if an epoch is complete
bool DistributedBuffer::IsEpochComplete() {
  return current_round_ >= matchings_.size();
}

bool DistributedBuffer::IsRoundComplete() {
  std::pair<int, int> current_state = machine_state_[current_round_][self_rank_];
  int partition_start1 = current_state.first * capacity_/2, partition_end1 = (current_state.first + 1) * capacity_/2;
  int partition_start2 = current_state.second * capacity_/2, partition_end2 = (current_state.second + 1) * capacity_/2;

  // 1. Check if all interactions between 2 super partitions are done
  // Check interactions within the same super partition only for the 0th round, as they won't happen in further rounds
  if(current_round_ == 0) {
    for(int i = partition_start1; i < partition_end1; i++) {
      for(int j = partition_start1; j < partition_end1; j++) {
        if(!interactions_matrix_[i][j]) return false;
      }
    }

    for(int i = partition_start2; i < partition_end2; i++) {
      for(int j = partition_start2; j < partition_end2; j++) {
        if(!interactions_matrix_[i][j]) return false;
      }
    }
  }

  for(int i = partition_start1; i < partition_end1; i++) {
    for(int j = partition_start2; j < partition_end2; j++) {
      if(!interactions_matrix_[i][j] || !interactions_matrix_[j][i]) return false;
    }
  }
  return true;
}

void DistributedBuffer::PrintInteractionMatrix() {
  std::cout << "Interaction matrix: " << std::endl;
  for(int i = 0; i < num_partitions_; i++) {
    for(int j = 0; j < num_partitions_; j++) {
      std::cout << interactions_matrix_[i][j] << "";
    }
    std::cout << std::endl;
  }
}

// ################## Constructor ##############################
DistributedBuffer::DistributedBuffer(DistributedBufferConfig config) {  
  self_rank_ = config.self_rank;
  num_partitions_ = config.num_partitions;

  capacity_ = config.capacity;
  num_workers_ = config.num_workers;
  server_address_ = config.server_addresses[self_rank_];
  server_addresses_ = config.server_addresses;
  interaction_edges_ = std::vector<std::vector<graph::InteractionEdges>>(num_partitions_, std::vector<graph::InteractionEdges>(num_partitions_));
  partitions_first_half_ = std::vector<graph::VertexPartition*>(capacity_/2, nullptr);
  partitions_second_half_ = std::vector<graph::VertexPartition*>(capacity_/2, nullptr);
  int n = num_workers_ * 2;
  matchings_.clear();
  GenerateMatchings(0, n, matchings_);
  GeneratePlan(matchings_, plan_, machine_state_, partition_to_be_sent_);
  current_round_ = 0;
  buffer_size_ = 0;
  fill_round_ = 1;
  partitions_fetched_ = std::vector<int>(matchings_.size(), 0);
  interactions_matrix_ = std::vector<std::vector<bool>>(num_partitions_, std::vector<bool>(num_partitions_, false));
  done_partitions_.clear();

  // Start the server.
  threads_.push_back(std::thread([&](){
    StartServer();
  }));

  // Setup client stubs to other servers. 
  SetupClientStubs();

  // Buffer starts working with self_rank_
  StartBuffer();
}
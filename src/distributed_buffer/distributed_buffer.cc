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

class PartitionServiceImpl final : public PartitionService::Service {
  private:
    DistributedBuffer* buffer_;
  public:
    explicit PartitionServiceImpl(DistributedBuffer* buffer) : buffer_(buffer) {}
    Status Ping(ServerContext* context, const PingRequest* request, PingResponse* response) {
      return Status::OK;
    }
    Status GetPartition(ServerContext* context, const PartitionRequest* request, PartitionResponse* response) {
      int super_partition_id = request->super_partition_id();
      graph::VertexPartition* partition = buffer_->SendPartition(super_partition_id);
      response->set_super_partition_id(super_partition_id);
      graph::VertexPartition* response_partition = response->mutable_partition();
      if(response_partition == nullptr) {
        *response_partition = *partition;
      }
      
      delete partition;
      return Status::OK;
    }
};

void DistributedBuffer::StartBuffer() {
  std::cout << "Rank: " << self_rank_ << std::endl;
  std::cout << "Starting the distributed buffer for rank " << self_rank_ << std::endl;
  // Ping all other servers
  for (int r = 0; r < num_workers_; r++) {
    if (r == self_rank_) continue;
    grpc::ClientContext context;
    PingResponse response;
    PingRequest request;
    Status status = client_stubs_[r]->Ping(&context, request, &response);
    std::cout << "Ping: rank: " << r << ", address: " << server_addresses_[r] << ", status ok? " << status.ok() << std::endl;
  }
}

void DistributedBuffer::StartServer() {
  PartitionServiceImpl service(this);
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  builder.SetMaxReceiveMessageSize(1 * 1024 * 1024 * 1024 + 1); // 1GB
  builder.SetMaxMessageSize(1 * 1024 * 1024 * 1024 + 1); // 1GB  
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  server->Wait();  
}

void DistributedBuffer::SetupClientStubs() {
  // setup client stubs
  for (int i = 0; i < num_workers_; i++) {
    grpc::ChannelArguments ch_args;
    ch_args.SetMaxReceiveMessageSize(-1);
    std::string target_str = server_addresses_[i];
    auto channel = grpc::CreateCustomChannel(target_str, grpc::InsecureChannelCredentials(), ch_args);
    client_stubs_.push_back(PartitionService::NewStub(channel));
  }
}

void DistributedBuffer::MarkInteraction(WorkUnit interaction) {
  // 1. Mark interaction as done
  int src_partition_id = interaction.src()->partition_id(), dst_partition_id = interaction.dst()->partition_id();
  interactions_matrix_[src_partition_id][dst_partition_id] = true;
  std::cout << "Interaction done: (" << src_partition_id << ", " << dst_partition_id << ")" << std::endl;

  // 2. Identify the outgoing partition and superpartition and check if it can be released.
  int outgoing_super_partition = partition_to_be_sent_[current_round_][self_rank_];
  int stable_super_partition = GetStablePartitionId();
  // 3. Check and try to release partition if it is part of outgoing super partition.
  CheckAndReleaseOutgoingPartition(outgoing_super_partition, stable_super_partition, src_partition_id);
  if(src_partition_id != dst_partition_id){
    CheckAndReleaseOutgoingPartition(outgoing_super_partition, stable_super_partition, dst_partition_id);
  }

  // Advance round if all interactions are done for this round
  if(IsRoundComplete()) {
    std::cout << "Round " << current_round_ << " complete" << std::endl;
    // current_round_++; // When to advance round??
  }
}

void DistributedBuffer::FillPartitions() {
  while(true) {
    // sleep if the buffer is full
    if(buffer_size_ == capacity_) std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get partition according to plan
    int target_machine = plan_[current_round_ + 1][self_rank_].first, target_super_partition = plan_[current_round_ + 1][self_rank_].second;
    grpc::ClientContext context;
    PartitionRequest request;
    request.set_super_partition_id(target_super_partition);
    PartitionResponse response;

    Status status = client_stubs_[target_machine]->GetPartition(&context, request, &response);
    if (!status.ok()) {
      std::cout << "GetPartition failed: " << status.error_code() << ": " << status.error_message() << std::endl;
      return;
    }
    std::cout << "GetPartition: super partition_id: " << response.super_partition_id() << std::endl;
    // Add partition to buffer
    graph::VertexPartition* partition = new graph::VertexPartition();
    *partition = response.partition();
    cout << "Partition vertices size: " << partition->vertices().size() << endl;
    AddPartitionToBuffer(partition);
    AddInteractions(partition);
  }
}

void DistributedBuffer::AddInteractions(graph::VertexPartition* partition) {
  int stable_super_partition_id = GetStablePartitionId();
  int incoming_partition_id = partition->partition_id();
  cout << "Stable super partition id: " << stable_super_partition_id << ", incoming partition id: " << incoming_partition_id << endl;
  int partition_start = stable_super_partition_id * (capacity_ /2), partition_end = (stable_super_partition_id + 1) * (capacity_ /2);

  // Determine which half is the stable super partition
  int start_idx = 0;
  if(vertex_partitions_[start_idx] != nullptr && vertex_partitions_[start_idx]->partition_id()  >= partition_start && vertex_partitions_[start_idx]->partition_id() < partition_end) {}
  else {
    start_idx = capacity_/2;
  }
  
  cout << "Acquire lock and insert undone interactions" << endl;
  cout << "Start idx in buffer: " << start_idx << endl;
  std::unique_lock<std::mutex> lock(mutex_);

  for(int i = start_idx; i < start_idx + capacity_/2; i++) {
    graph::VertexPartition* vp = vertex_partitions_[i];
    if(!interactions_matrix_[incoming_partition_id][vp->partition_id()]) {
      WorkUnit interaction(partition, vp, &interaction_edges_[incoming_partition_id][vp->partition_id()]);
      interaction_queue_.push(interaction);
      cout << "Inserted interaction: (" << incoming_partition_id << ", " << vp->partition_id() << ")" << endl;
    }
    if(!interactions_matrix_[vp->partition_id()][incoming_partition_id]) {
      WorkUnit interaction(vp, partition, &interaction_edges_[vp->partition_id()][incoming_partition_id]);
      interaction_queue_.push(interaction);
      cout << "Inserted interaction: (" << vp->partition_id() << ", " << incoming_partition_id << ")" << endl;
    }
  }
  cv_.notify_one();
}


// @TODO: change shared ptr for interaction to unique ptr.
std::optional<WorkUnit> DistributedBuffer::GetWorkUnit() {
  std::unique_lock<std::mutex> lock(mutex_);
  if(interaction_queue_.empty() && !IsEpochComplete()) {
    std::cout << "Epoch not complete yet, but no workunits available. Waiting on CV till queue populated" << std::endl;
    cv_.wait(lock); // release lock and wait till queue populated
  }

  auto interaction = interaction_queue_.front();
  interaction_queue_.pop();
  MarkInteraction(interaction);
  return interaction;
}

bool DistributedBuffer::IsEpochComplete() {
  for(int i = 0; i < num_partitions_; i++) {
    for(int j = 0; j < num_partitions_; j++) {
      if(!interactions_matrix_[i][j]) return false;
    }
  }
  return true;
}

bool DistributedBuffer::IsRoundComplete() {
  std::pair<int, int> current_state = machine_state_[current_round_][self_rank_];
  int partition_start1 = current_state.first * capacity_/2, partition_end1 = (current_state.first + 1) * capacity_/2;
  int partition_start2 = current_state.second * capacity_/2, partition_end2 = (current_state.second + 1) * capacity_/2;

  // If all interactions between 2 super partitions are done --> round complete
  for(int i = partition_start1; i < partition_end1; i++) {
    for(int j = partition_start2; j < partition_end2; j++) {
      if(!interactions_matrix_[i][j]) return false;
    }
  }
  return true;
}


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
  interactions_matrix_ = std::vector<std::vector<bool>>(num_partitions_, std::vector<bool>(num_partitions_, false));
  done_partitions_.clear();

  // Start the server.
  threads_.push_back(std::thread([&](){
    StartServer();
  }));

  // Setup client stubs to other servers. 
  SetupClientStubs();

  // Buffer starts working with self_rank_
  threads_.push_back(std::thread([&](){
    StartBuffer();
  }));
}
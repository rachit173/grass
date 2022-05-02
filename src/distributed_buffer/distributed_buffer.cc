#include "distributed_buffer.h"

using namespace std;

// Pick work unit from queue
std::optional<WorkUnit> DistributedBuffer::GetWorkUnit() {
  std::unique_lock<std::mutex> lock(mutex_);
  if(interaction_queue_.empty() && !IsEpochComplete()) {
    spdlog::debug("Waiting for work unit");
    cv_.wait(lock); // release lock and wait till queue populated
  }

  else if(IsEpochComplete()) {
    spdlog::debug("Epoch complete");
    return std::nullopt;
  }

  auto interaction = interaction_queue_.front();
  spdlog::trace("Round: {}, Work Unit: (src: {}, dst: {})", current_round_, interaction.src()->partition_id(), interaction.dst()->partition_id());
  interaction_queue_.pop();
  return interaction;
}

void DistributedBuffer::MarkInteraction(WorkUnit interaction) {
  // 1. Mark interaction as done
  int src_partition_id = interaction.src()->partition_id(), dst_partition_id = interaction.dst()->partition_id();
  interactions_matrix_[src_partition_id][dst_partition_id] = true;
  spdlog::trace("Marked interaction: (src: {}, dst: {})", src_partition_id, dst_partition_id);

  // 2. Identify the outgoing partition and superpartition and check if it can be released.
  int outgoing_super_partition = partition_to_be_sent_[current_round_][self_rank_];
  int stable_super_partition = GetStablePartitionId(current_round_);
  
  // Check and release only it is not the last round
  if(current_round_ < (int) (partition_to_be_sent_.size() - 1)) {
    // 3. Check and try to release partition if it is part of outgoing super partition.
    CheckAndReleaseOutgoingPartition(outgoing_super_partition, stable_super_partition, src_partition_id);
    if(src_partition_id != dst_partition_id) {
      CheckAndReleaseOutgoingPartition(outgoing_super_partition, stable_super_partition, dst_partition_id);
    }
  }

  // Advance round if all interactions are done for this round
  if(IsRoundComplete()) {
    spdlog::debug("Round {} completed", current_round_);
    current_round_++; // TODO: Might have to put a lock around it as it's read in a different thread as well
  }
}

// Write to a file or store to some different buffer to be retrieved when sending to another part of the buffer.
void DistributedBuffer::ReleasePartition(int super_partition_id, int partition_id) {
  auto comp_partition = [&](graph::VertexPartition* p) {
    return p != nullptr && p->partition_id() == partition_id;
  };

  auto partition_iter = std::find_if(vertex_partitions_.begin(), vertex_partitions_.end(), comp_partition);

  if (partition_iter == vertex_partitions_.end()) {
    spdlog::error("Partition {} not found. Exiting..", partition_id);
    exit(1);
  }

  graph::VertexPartition* partition = *partition_iter;
  
  // Remove partition from the list of partitions and decrease partition count
  *partition_iter = nullptr;
  buffer_size_--;

  std::unique_lock<std::mutex> lock(mutex_send_);
  done_partitions_[partition_id] = partition;

  spdlog::trace("Released partition: {}", partition_id);
  cv_send_.notify_one(); // Notify the gRPC thread that a partition is available
}

// Check if the partition is ready to be released.
void DistributedBuffer::CheckAndReleaseOutgoingPartition(int outgoing_super_partition_id, int stable_super_partition_id, int partition_id) {

  if(!BelongsToSuperPartition(partition_id, outgoing_super_partition_id)) {
    spdlog::trace("Partition {} does not belong to outgoing super partition {}", partition_id, outgoing_super_partition_id);
    return;
  }
  
  bool partition_done = true;
  
  // Check if interactions are complete with the stable super partition as well as inside the outgoing super partition
  auto stable_partition_range = GetPartitionRange(stable_super_partition_id);

  for(int i = stable_partition_range.first; i < stable_partition_range.second; i++) {
    if(!(interactions_matrix_[partition_id][i] && interactions_matrix_[i][partition_id])) {
      partition_done = false;
      break;
    }
  }

  // Partition belongs to outgoing super partition. Check interactions with own super partition only for the first round 
  // (As that will only be executed for the first round).
  if(current_round_ == 0) {
    auto outgoing_partition_range = GetPartitionRange(outgoing_super_partition_id);
    for(int i = outgoing_partition_range.first; i < outgoing_partition_range.second; i++) {
      if(! (interactions_matrix_[partition_id][i] && interactions_matrix_[i][partition_id]) ) {
        partition_done = false;
        break;
      }
    }
  }

  PrintInteractionMatrix();
  if(!partition_done) {
    spdlog::trace("Partition {} not done yet", partition_id);
    return;
  }

  spdlog::debug("Partition {} is done", partition_id);
  ReleasePartition(outgoing_super_partition_id, partition_id);
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
  interactions_matrix_ = std::vector<std::vector<bool>>(num_partitions_, std::vector<bool>(num_partitions_, false));

  int num_super_partitions = num_workers_ * 2;
  matchings_.clear();
  GenerateMatchings(0, num_super_partitions, matchings_);
  GeneratePlan(matchings_, plan_, machine_state_, partition_to_be_sent_);

  current_round_ = 0;
  buffer_size_ = 0;
  fill_round_ = 1;

  partitions_fetched_ = std::vector<int>(matchings_.size(), 0);
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
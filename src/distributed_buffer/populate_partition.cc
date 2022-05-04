#include "distributed_buffer.h"

void DistributedBuffer::PopulatePartitions() {
  while(true) {
    // sleep if the buffer is full or if all partitions are populated for this epoch
    while(buffer_size_ == capacity_ || fill_round_ == rounds_per_iteration_){
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Get partition according to plan
    int target_machine = plan_[fill_round_][self_rank_].first, target_super_partition = plan_[fill_round_][self_rank_].second;
    spdlog::trace("Populating Round {}: Fetching partition {} from machine {}", fill_round_, target_super_partition, target_machine);

    grpc::ClientContext context;
    graph::PartitionRequest request;
    request.set_super_partition_id(target_super_partition);
    graph::PartitionResponse response;

    grpc::Status status = client_stubs_[target_machine]->GetPartition(&context, request, &response);
    if (!status.ok()) {
      spdlog::trace("[gRPC] Failed to get partition {} from machine {}. Error: {}", target_super_partition, target_machine, status.error_message());
      sleep(1);
      continue; // Try again if failed, don't return
    }

    // Add partition to buffer
    graph::VertexPartition* partition = new graph::VertexPartition();
    *partition = response.partition();
    
    spdlog::trace("Partition {} fetched from machine {}", partition->partition_id(), target_machine);

    // Add partition to buffer and interactions corresponding to the new partition
    AddPartitionToBuffer(partition);
    AddInteractions(partition);

    // Fetch capacity_/2 partitions from other machines in every round
    partitions_fetched_[fill_round_]++;
    if(partitions_fetched_[fill_round_] == (capacity_/2)) {
      fill_round_++;
      if(fill_round_ < rounds_per_iteration_) {
        partitions_fetched_[fill_round_] = 0;
      }
    }
  }
}

// Add a new partition to the buffer (vertex_partitions_)
void DistributedBuffer::AddPartitionToBuffer(graph::VertexPartition* partition) {
  std::unique_lock<std::mutex> buffer_lock(buffer_mutex_);
  auto comp_partition = [&](graph::VertexPartition* p) {
    return p == nullptr;
  };

  auto partition_iter = std::find_if(vertex_partitions_.begin(), vertex_partitions_.end(), comp_partition);

  if (partition_iter == vertex_partitions_.end()) {
    spdlog::error("Buffer Full. Cannot add partition {}", partition->partition_id());
    exit(1);
  }

  *partition_iter = partition;
  buffer_size_++;

  spdlog::trace("Added partition {} to buffer. New buffer size: {}", partition->partition_id(), buffer_size_);
}

void DistributedBuffer::AddInteractions(graph::VertexPartition* partition) {
  int stable_super_partition_id = GetStablePartitionId(fill_round_ - 1);
  int incoming_partition_id = partition->partition_id();

  std::unique_lock<std::mutex> buffer_lock(buffer_mutex_);
  // Determine which half is the stable super partition
  int start_idx = 0;
  if(vertex_partitions_[start_idx] == nullptr || !BelongsToSuperPartition(vertex_partitions_[start_idx]->partition_id(), stable_super_partition_id)) {
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
      spdlog::trace("Added interaction between partition {} and {}", incoming_partition_id, vp->partition_id());
    }
    if(!interactions_matrix_[vp->partition_id()][incoming_partition_id]) {
      WorkUnit interaction(vp, partition, &interaction_edges_[vp->partition_id()][incoming_partition_id]);
      interaction_queue_.push(interaction);
      added_interactions++;
      spdlog::trace("Added interaction between partition {} and {}", vp->partition_id(), incoming_partition_id);
    }
  }
  spdlog::trace("Added {} interactions", added_interactions);
  cv_.notify_one(); // Notify the main thread that is waiting for interactions to be added to the queue
}

#include "distributed_buffer.h"

using namespace std;

 void DistributedBuffer::LoadInteractions(init_interactions_func_t init_interactions_func) {
    init_interactions_func();
 }

partition::Partition* DistributedBuffer::InitPartition(init_partition_func_t init_partition_func, int partition_id, int partition_start, int partition_end) {
  partition::Partition* partition = new partition::Partition();
  partition->set_partition_id(partition_id);

  init_partition_func(partition, partition_start, partition_end);
  return partition;
}

void DistributedBuffer::InitSuperPartition(init_partition_func_t init_partition_func, std::vector<partition::Partition*>& partitions, int super_partition_id) {
  int B = capacity_/2;
  for(int i = 0; i < B; i++) {
    int partition_id = super_partition_id * B + i;
    int partition_start = partition_id * partition_size_, partition_end = std::min(num_vertices_, (int64_t)( (partition_id + 1) * partition_size_) ); // Put cap on end at num_vertices_

    if(partition_id >= num_partitions_) {
      throw std::invalid_argument(fmt::format("DistributedBuffer::InitSuperPartition: partition_id: {} is out of range", partition_id));
    }
    
    if (partition_end < partition_start) {
      throw std::invalid_argument(fmt::format("DistributedBuffer::InitSuperPartition: partition_end: {} is less than partition_start: {}", partition_end, partition_start));
    }
    
    partitions[i] = InitPartition(init_partition_func, partition_id, partition_start, partition_end); // [partition_start, partition_end)
    buffer_size_++; // Track buffer size
  }
}

void DistributedBuffer::LoadInitialPartitions(init_partition_func_t init_partition_func) {
  spdlog::debug("Initialize partitions.");

  if (!partitions_.empty()) {
    spdlog::debug("Partitions are already loaded.");
    return;
  }

  partition_vertices_.resize(num_partitions_);
  for (int i = 0; i < num_vertices_; i++) {
    int partition_id = GetPartitionHash(i);
    partition_vertices_[partition_id].push_back(i);
  }

  vvii matchings;
  GenerateMatchings(0, 2 * num_workers_, matchings);
  pair<int, int> initial_partition_state = matchings[0][self_rank_];
  matchings.clear();
  int left_super_partition = initial_partition_state.first; // machine_state_[0][self_rank_].first;
  int right_super_partition = initial_partition_state.second; // machine_state_[0][self_rank_].second;
  int B = capacity_/2;
  partitions_first_half_ = std::vector<partition::Partition*>(capacity_/2, nullptr);
  partitions_second_half_ = std::vector<partition::Partition*>(capacity_/2, nullptr);

  InitSuperPartition(init_partition_func, partitions_first_half_, left_super_partition);
  InitSuperPartition(init_partition_func, partitions_second_half_, right_super_partition);
  
  // whole view of vertex partitions in buffer
  partitions_ = std::vector<partition::Partition*>(capacity_, nullptr);
  for(int i = 0; i < B; i++) {
    partitions_[i] = partitions_first_half_[i];
    partitions_[i + B] = partitions_second_half_[i];
  }
}

void DistributedBuffer::ProduceInteractions() {
  for(int i = 0; i < capacity_; i++) {
    for(int j = 0; j < capacity_; j++) {
      partition::Partition* src = partitions_[i];
      partition::Partition* dst = partitions_[j];
      partition::Interaction* interEdges = nullptr;
      if (!interactions_.empty()) {
        interEdges = &interactions_[src->partition_id()][dst->partition_id()];
      }
      WorkUnit interaction(src, dst, interEdges);
      interaction_queue_.push(interaction);
    }
  }
}
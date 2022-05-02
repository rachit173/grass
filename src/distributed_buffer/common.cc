#include "distributed_buffer.h"

int DistributedBuffer::GetStablePartitionId(int round) {
    int outgoing_super_partition = partition_to_be_sent_[round][self_rank_];
    int stable_super_partition = machine_state_[round][self_rank_].first;
    if(machine_state_[round][self_rank_].first == outgoing_super_partition){
        stable_super_partition = machine_state_[round][self_rank_].second;
    }
    return stable_super_partition;
}

void DistributedBuffer::PrintInteractionMatrix() {
  std::stringstream ss;
  for(int i = 0; i < num_partitions_; i++) {
    for(int j = 0; j < num_partitions_; j++) {
      ss << interactions_matrix_[i][j] << " ";
    }
    ss << "\n";
  }
  spdlog::trace("Interaction matrix: \n{}", ss.str());
}

// Determine if an epoch is complete
bool DistributedBuffer::IsEpochComplete() {
  return current_round_ >= (int) matchings_.size();
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

// Determine if the partition id belongs to super partition id
bool DistributedBuffer::BelongsToSuperPartition(int partition_id, int super_partition_id) {
  // Partition should belong to outgoing super partition to be eligible for release.
  std::pair<int, int> range = GetPartitionRange(super_partition_id);
  return (partition_id >= range.first) && (partition_id < range.second);
}

// Return the range of partitions belonging to a super partition [start, end)
std::pair<int, int> DistributedBuffer::GetPartitionRange(int super_partition_id) {
  int partition_start = super_partition_id * (capacity_/2);
  int partition_end = (super_partition_id + 1) * (capacity_/2);
  return std::make_pair(partition_start, partition_end);
}
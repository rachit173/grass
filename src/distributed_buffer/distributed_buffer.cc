#include "distributed_buffer.h"

using namespace std;

// Pick work unit from queue
std::optional<WorkUnit> DistributedBuffer::GetWorkUnit() {
  Timer timer;
  timer.start();
  std::unique_lock<std::mutex> lock(mutex_);
  if(interaction_queue_.empty() && !IsEpochComplete()) {
    spdlog::debug("Waiting for work unit");
    cv_.wait(lock); // release lock and wait till queue populated
  }
  if(IsEpochComplete()) {
    return std::nullopt;
  }
  auto interaction = interaction_queue_.front();
  spdlog::trace("Round: {}, Work Unit: (src: {}, dst: {})", outgoing_round_, interaction.src()->partition_id(), interaction.dst()->partition_id());
  interaction_queue_.pop();
  timer.stop();
  metric_get_work_unit_.add(timer.get_time_in_nanoseconds());
  return interaction;
}

void DistributedBuffer::MarkInteraction(WorkUnit interaction) {
  // 1. Mark interaction as done
  int src_partition_id = interaction.src()->partition_id(), dst_partition_id = interaction.dst()->partition_id();
  interactions_matrix_[src_partition_id][dst_partition_id] = true;
  spdlog::trace("Marked interaction: (src: {}, dst: {})", src_partition_id, dst_partition_id);

  CheckAndReleaseAllPartitions();
}

// Write to a file or store to some different buffer to be retrieved when sending to another part of the buffer.
void DistributedBuffer::ReleasePartition(int partition_id) {
  partition::Partition* partition;
  {
    std::unique_lock<std::mutex> buffer_lock(buffer_mutex_);
    // Remove a partition with a particular partition id from the buffer and put it in hashmap (TODO: Save to file)
    auto comp_partition = [&](partition::Partition* p) {
      return p != nullptr && p->partition_id() == partition_id;
    };
    auto partition_iter = std::find_if(partitions_.begin(), partitions_.end(), comp_partition);
    if (partition_iter == partitions_.end()) {
      spdlog::debug("Partition {} not found. Exiting..", partition_id);
      return;
    }
    partition = *partition_iter;
    
    // Remove partition from the list of partitions and decrease partition count
    *partition_iter = nullptr;
    buffer_size_--;
  }

  std::unique_lock<std::mutex> lock(mutex_send_);
  done_partitions_[partition_id] = partition;

  spdlog::trace("Released partition: {}", partition_id);
  cv_send_.notify_one(); // Notify the gRPC thread that a partition is available
}

void DistributedBuffer::CheckAndReleaseAllPartitions() {
  for(int i = 0; i < capacity_; i++) {
    if(partitions_[i] == nullptr) continue;
    CheckAndReleaseOutgoingPartition(partitions_[i]->partition_id());
  }
}

// Check if the partition is ready to be released.
void DistributedBuffer::CheckAndReleaseOutgoingPartition(int partition_id) {
  // Identify the outgoing partition and superpartition and check if it can be released.
  int outgoing_super_partition_id = partition_to_be_sent_[outgoing_round_][self_rank_];
  int stable_super_partition_id = GetStablePartitionId(outgoing_round_);
  
  // Check and release a partition only if it is not the last round in the epoch
  // Reason: Apply phase & next epoch requires all partitions present in the buffer
  if(outgoing_round_ % rounds_per_iteration_ == rounds_per_iteration_ - 1) {
    spdlog::trace("Last round, don't release");
    return;
  }

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
  if(outgoing_round_ % rounds_per_iteration_ == 0) {
    auto outgoing_partition_range = GetPartitionRange(outgoing_super_partition_id);
    for(int i = outgoing_partition_range.first; i < outgoing_partition_range.second; i++) {
      if(! (interactions_matrix_[partition_id][i] && interactions_matrix_[i][partition_id]) ) {
        partition_done = false;
        break;
      }
    }
  }

  if(!partition_done) {
    spdlog::trace("Partition {} not done yet", partition_id);
    return;
  }

  spdlog::debug("Partition {} is done", partition_id);
  ReleasePartition(partition_id);
}

void DistributedBuffer::InitEpoch() {
  // Initialize data members
  outgoing_round_ = 0;
  partitions_sent_ = 0;
  fill_round_ = 1;
  epoch_complete_ = false;
  interactions_matrix_ = std::vector<std::vector<bool>>(num_partitions_, std::vector<bool>(num_partitions_, false));
  partitions_fetched_ = 0;
  pair<int, int> current_partitions = {partitions_[0]->partition_id()/ (capacity_/2),
                                   partitions_[capacity_/2]->partition_id()/ (capacity_/2)};

  // Assert that the ones not re-initialized have the expected values
  assert(buffer_size_ == capacity_);
  assert(done_partitions_.empty());
  
  // Generate matchings for each epoch
  plan_.clear();
  partition_to_be_sent_.clear();

  if(matchings_.empty()) { // Generating matchings and plan for the first time
    int num_super_partitions = num_workers_ * 2;
    GenerateMatchings(0, num_super_partitions, matchings_);
    machine_state_.clear();
    GeneratePlan(matchings_, plan_, machine_state_, partition_to_be_sent_);
  } else { // Assume that the matchings are already generated as the buffer has a certain state
    vector<pair<int, int>> current_matching = machine_state_.back();
    vector<pair<int, int>> current_machine_state = machine_state_.back();
    // Rearrange the buffer to reflect expected machine state if flipped
    if(current_machine_state[self_rank_] != current_partitions) {
      RearrangeBuffer();
      current_partitions = make_pair(partitions_[0]->partition_id()/ (capacity_/2),
                                   partitions_[capacity_/2]->partition_id()/ (capacity_/2));
    }
    assert(current_machine_state[self_rank_] == current_partitions);
    // Get the next matchings in a cyclic way starting from the current matching
    matchings_.pop_back();
    matchings_.insert(matchings_.begin(), current_matching);
    // Get the next plan and machine states using the updated matchings and current machine state
    machine_state_.clear();
    GeneratePlan(matchings_, plan_, machine_state_, partition_to_be_sent_, current_machine_state);
  }
  
  // Insert the initial interactions in the queue according to the buffer contents
  ProduceInteractions();
}

DistributedBuffer::DistributedBuffer(DistributedBufferConfig config, PartitionType partition_type) {  
  self_rank_ = config.self_rank;
  num_partitions_ = config.num_partitions;
  capacity_ = config.capacity;
  num_workers_ = config.num_workers;
  server_address_ = config.server_addresses[self_rank_];
  server_addresses_ = config.server_addresses;
  rounds_per_iteration_ = 2 * num_workers_ - 1;
  buffer_size_ = 0;
  partition_type_ = partition_type;
  started_ = false;
  matchings_.clear();
  machine_state_.clear();
  done_partitions_.clear();
}

void DistributedBuffer::Load(init_interactions_func_t init_interactions_func, init_partition_func_t init_partition_func) {
  Timer timer;
  timer.start();
  LoadInteractions(init_interactions_func);
  LoadInitialPartitions(init_partition_func);
  timer.stop();
  metric_load_data_.add(timer.get_time_in_nanoseconds());
}

void DistributedBuffer::Start() {
  spdlog::info("Starting distributed buffer");
  //check if threads are already running
  
  if(started_) {
    spdlog::trace("Distributed buffer already started");
    return;
  }
  
  threads_.push_back(std::thread([&](){
    StartServer();
  }));
  threads_.push_back(std::thread([&](){
    PopulatePartitions();
  }));
  
  SetupClientStubs();
  PingAll();

  started_ = true;
}

void DistributedBuffer::GetVertexPartitions(std::vector<graph::VertexPartition*> &vertex_partitions) {
  vertex_partitions.clear();
  for(partition::Partition *partition : partitions_) {
      vertex_partitions.emplace_back(partition->mutable_vertex_partition());
  }
}

void DistributedBuffer::GetMatrixPartitions(std::vector<matmul::MatrixPartition*> &matrix_partitions) {
  matrix_partitions.clear();
  for(partition::Partition *partition : partitions_) {
      matrix_partitions.emplace_back(partition->mutable_matrix_partition());
  }
}

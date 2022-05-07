#ifndef DISTRIBUTED_BUFFER_H
#define DISTRIBUTED_BUFFER_H

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include "protos/partition.grpc.pb.h"

#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <fstream>
#include <sstream>
#include <functional>
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <assert.h>
#include <functional>

#include "matching_generator.h"

typedef partition::Partition::PartitionTypeCase PartitionType;
typedef std::function<void ()> init_interactions_func_t;
typedef std::function<void (partition::Partition *, int, int)> init_partition_func_t;

struct DistributedBufferConfig {
  int self_rank;
  int num_partitions;
  int capacity;
  int num_workers;
  std::vector<std::string> server_addresses;
  int partition_size;
};

class WorkUnit {
public:
  WorkUnit(partition::Partition* src_partition, 
              partition::Partition* dst_partition, 
              partition::Interaction* edges) : 
              src_partition_(src_partition), dst_partition_(dst_partition), edges_(edges) {}
  partition::Partition* src(){ return src_partition_; }
  partition::Partition* dst(){ return dst_partition_; }
  partition::Interaction* edges(){ return edges_; }

private:  
  partition::Partition* src_partition_;
  partition::Partition* dst_partition_;
  partition::Interaction* edges_;
};


class DistributedBuffer {
public:
  explicit DistributedBuffer(DistributedBufferConfig config, PartitionType partition_type);
  // Used by Graph
  void Start();
  void Load(init_interactions_func_t init_interactions_func, init_partition_func_t init_partition_func);
  void InitEpoch();
  std::optional<WorkUnit> GetWorkUnit();
  void MarkInteraction(WorkUnit interaction);
  void GetVertexPartitions(std::vector<graph::VertexPartition*> &vertex_partitions);
  void GetMatrixPartitions(std::vector<matmul::MatrixPartition*> &matrix_partitions);
  std::vector<std::vector<partition::Interaction>> &GetInteractions() { return interactions_; }
  int GetNumPartitions() { return num_partitions_; }
  int64_t GetPartitionSize() { return partition_size_; }
  void SetPartitionSize(int64_t partition_size) { partition_size_ = partition_size; }
  int64_t GetNumVertices() { return num_vertices_; }
  void SetNumVertices(int64_t num_vertices) { num_vertices_ = num_vertices; }
  int64_t GetNumEdges() { return num_edges_; }
  int64_t GetOutgoingRound() { return outgoing_round_; }
  void SetNumEdges(int64_t num_edges) { num_edges_ = num_edges; }
  PartitionType GetPartitionType() { return partition_type_; }
  void WaitForEpochCompletion();
  // Used by Partition service
  partition::Partition* SendPartition(int partition_id);
  std::vector<int> &GetPartitionVertices(int partition_id) { return partition_vertices_[partition_id]; }
  uint64_t GetPartitionHash(int partition_id) { return hasher_(partition_id) % num_partitions_; }
  
private:
  void LoadInteractions(init_interactions_func_t init_interactions_func);
  void LoadInitialPartitions(init_partition_func_t init_partition_func);
  void StartServer();
  void PingAll();
  void SetupClientStubs();
  int GetStablePartitionId(int round);
  void ReleasePartition(int partition_id);
  void CheckAndReleaseOutgoingPartition(int partition_id);
  void CheckAndReleaseAllPartitions();
  void AddPartitionToBuffer(partition::Partition* partition);
  bool IsEpochComplete();
  bool IsInteractionsDone();
  bool BelongsToSuperPartition(int partition_id, int super_partition_id);
  void PopulatePartitions();
  void AddInteractions(partition::Partition* partition); 
  void PrintInteractionMatrix();
  partition::Partition* InitPartition(init_partition_func_t init_partition_func, int partition_id, int partition_start, int partition_end);
  void InitSuperPartition(init_partition_func_t init_partition_func, std::vector<partition::Partition*>& super_partition, int super_partition_id);
  std::pair<int, int> GetPartitionRange(int super_partition_id);
  void ProduceInteractions();
  void NotifyEpochComplete();
  void RearrangeBuffer();

  PartitionType partition_type_;
  int64_t num_vertices_;
  int64_t num_edges_;
  int num_iterations_;
  int rounds_per_iteration_;
  int self_rank_;
  int num_partitions_;
  int capacity_;
  int num_workers_;
  int partition_size_;
  int outgoing_round_;
  int partitions_sent_;
  int fill_round_;
  int partitions_fetched_;
  int buffer_size_;
  bool epoch_complete_;
  std::hash<int> hasher_;
  std::vector<std::vector<int>> partition_vertices_;
  bool started_;
  std::vector<std::vector<std::pair<int, int>>> matchings_;
  std::vector<std::vector<std::pair<int, int>>> plan_;
  std::vector<std::vector<std::pair<int, int>>> machine_state_;
  std::vector<std::vector<int>> partition_to_be_sent_;
  std::string server_address_;
  std::vector<std::thread> threads_;
  std::vector<std::vector<partition::Interaction>> interactions_;
  std::vector<partition::Partition*> partitions_first_half_;
  std::vector<partition::Partition*> partitions_second_half_;
  std::vector<partition::Partition*> partitions_;
  std::unordered_map<int, partition::Partition*> done_partitions_;
  std::vector<std::vector<bool>> interactions_matrix_;
  std::vector<std::pair<int, int>> super_partitiion_order_;
  std::vector<std::string> server_addresses_;
  std::vector<std::unique_ptr<partition::PartitionService::Stub>> client_stubs_;
  std::queue<WorkUnit> interaction_queue_;
  std::condition_variable cv_start_;
  std::mutex mutex_start_;
  std::condition_variable cv_;
  std::mutex mutex_;
  std::condition_variable cv_send_;
  std::mutex mutex_send_;
  std::condition_variable cv_epoch_completion_;
  std::mutex mutex_epoch_completion_;
  std::mutex buffer_mutex_;
};
#endif // DISTRIBUTED_BUFFER_H
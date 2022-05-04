#ifndef DISTRIBUTED_BUFFER_H
#define DISTRIBUTED_BUFFER_H

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include "protos/graph.grpc.pb.h"

#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <sstream>
#include <fstream>
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <assert.h>

#include "matching_generator.h"

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
  WorkUnit(graph::VertexPartition* src_partition, 
              graph::VertexPartition* dst_partition, 
              graph::InteractionEdges* edges) : 
              src_partition_(src_partition), dst_partition_(dst_partition), edges_(edges) {}
  graph::VertexPartition* src(){ return src_partition_; }
  graph::VertexPartition* dst(){ return dst_partition_; }
  graph::InteractionEdges* edges(){ return edges_; }

private:  
  graph::VertexPartition* src_partition_;
  graph::VertexPartition* dst_partition_;
  graph::InteractionEdges* edges_;
};


class DistributedBuffer {
public:
  explicit DistributedBuffer(DistributedBufferConfig config, std::string& graph_filepath, bool weighted_edges = false);
  // Used by Graph
  void InitEpoch();
  std::optional<WorkUnit> GetWorkUnit();
  void MarkInteraction(WorkUnit interaction);
  std::vector<graph::VertexPartition*>& GetPartitions() { return vertex_partitions_; }
  int64_t GetPartitionSize() { return partition_size_; }
  int64_t GetNumVertices() { return num_vertices_; }
  int64_t GetNumEdges() { return num_edges_; }
  void WaitForEpochCompletion();
  // Used by Partition service
  graph::VertexPartition* SendPartition(int partition_id);
  
private:
  void StartServer();
  void PingAll();
  void LoadInteractionEdges(std::string& graph_file, bool weighted_edges);
  void LoadInitialPartitions();
  void SetupClientStubs();
  int GetStablePartitionId(int round);
  void ReleasePartition(int partition_id);
  void CheckAndReleaseOutgoingPartition(int outgoing_super_partition_id, int stable_super_partition_id, int partition_id);
  void AddPartitionToBuffer(graph::VertexPartition* partition);
  bool IsEpochComplete();
  bool IsRoundComplete();
  bool BelongsToSuperPartition(int partition_id, int super_partition_id);
  void PopulatePartitions();
  void AddInteractions(graph::VertexPartition* partition); 
  void PrintInteractionMatrix();
  graph::VertexPartition* InitPartition(int partition_id, int partition_start, int partition_end);
  void InitSuperPartition(std::vector<graph::VertexPartition*>& super_partition, int super_partition_id);
  std::pair<int, int> GetPartitionRange(int super_partition_id);
  void ProduceInteractions();
  void NotifyPartitionSent();

  int64_t num_vertices_;
  int64_t num_edges_;
  int num_iterations_;
  int rounds_per_iteration_;
  int self_rank_;
  int num_partitions_;
  int capacity_;
  int num_workers_;
  int partition_size_;
  int current_round_;
  int fill_round_;
  int buffer_size_;
  bool epoch_complete_;
  std::vector<std::vector<std::pair<int, int>>> matchings_;
  std::vector<std::vector<std::pair<int, int>>> plan_;
  std::vector<std::vector<std::pair<int, int>>> machine_state_;
  std::vector<std::vector<int>> partition_to_be_sent_;
  std::string server_address_;
  std::vector<std::thread> threads_;
  std::vector<std::vector<graph::InteractionEdges>> interaction_edges_;
  std::vector<graph::VertexPartition*> partitions_first_half_;
  std::vector<graph::VertexPartition*> partitions_second_half_;
  std::vector<graph::VertexPartition*> vertex_partitions_;
  std::unordered_map<int, graph::VertexPartition*> done_partitions_;
  std::vector<int> partitions_fetched_;
  std::vector<std::vector<bool>> interactions_matrix_;
  std::vector<std::pair<int, int>> super_partitiion_order_;
  std::vector<std::string> server_addresses_;
  std::vector<std::unique_ptr<graph::PartitionService::Stub>> client_stubs_;
  std::queue<WorkUnit> interaction_queue_;
  std::condition_variable cv_;
  std::mutex mutex_;
  std::condition_variable cv_send_;
  std::mutex mutex_send_;
  std::condition_variable cv_epoch_completion_;
  std::mutex mutex_epoch_completion_;
  std::mutex buffer_mutex_;
};
#endif // DISTRIBUTED_BUFFER_H
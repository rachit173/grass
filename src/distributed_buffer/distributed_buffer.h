#ifndef DISTRIBUTED_BUFFER_H
#define DISTRIBUTED_BUFFER_H

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>

#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif


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


void GenerateMatchings(int l, int r, std::vector<std::vector<std::pair<int, int>>>& matchings);
void GeneratePlan(std::vector<std::vector<std::pair<int, int>>>& matchings, 
                  std::vector<std::vector<std::pair<int, int>>>& plan, 
                  std::vector<std::vector<std::pair<int, int>>>& machine_state,
                  std::vector<std::vector<int>>& partition_to_be_sent);
class DistributedBuffer {
public:
  explicit DistributedBuffer(DistributedBufferConfig config); 
  void StartServer();
  void StartBuffer();
  int LoadInteractionEdges(std::string& graph_file, bool weighted_edges, std::vector<graph::Edge*>& edges);
  void LoadInitialPartitions();
  void SetupClientStubs();
  std::vector<graph::VertexPartition*>& get_partitions();
  std::optional<WorkUnit> GetWorkUnit();
  void ProduceInteractions();
  graph::VertexPartition* SendPartition(int partition_id);
  void ClearPartition(int partition_id);
  void MarkInteraction(WorkUnit interaction);
  void notifyEpochComplete();
  void waitForEpochCompletion();

private:
  int GetStablePartitionId(int round);
  void ReleasePartition(int super_partition_id, int partition_id);
  void CheckAndReleaseOutgoingPartition(int outgoing_super_partition_id, int stable_super_partition_id, int partition_id);
  void AddPartitionToBuffer(graph::VertexPartition* partition);
  
  bool IsEpochComplete();
  bool IsRoundComplete();
  void FillPartitions();
  void AddInteractions(graph::VertexPartition* partition);
  void PrintInteractionMatrix();
  
  graph::VertexPartition* InitPartition(int partition_id, int partition_start, int partition_end);
  void InitSuperPartition(std::vector<graph::VertexPartition*>& super_partition, int super_partition_id);

  int self_rank_;
  int num_partitions_;
  int capacity_;
  int num_workers_;
  int partition_size_;
  int current_round_;
  int fill_round_;
  int buffer_size_;
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
  std::condition_variable cv_round_;
  std::mutex mutex_;
  std::condition_variable cv_send_;
  std::mutex mutex_send_;
  std::condition_variable cv_epoch_completion_;
  std::mutex mutex_epoch_completion_;
};
#endif // DISTRIBUTED_BUFFER_H
// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>

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
};

void GenerateMatchings(int l, int r, std::vector<std::vector<std::pair<int, int>>>& matchings);
void GeneratePlan(std::vector<std::vector<std::pair<int, int>>>& matchings, 
                  std::vector<std::vector<std::pair<int, int>>>& plan, 
                  std::vector<std::vector<std::pair<int, int>>>& machine_state);
class DistributedBuffer {
 public:
  explicit DistributedBuffer(DistributedBufferConfig config); 
  void StartServer();
  void StartBuffer();
  void LoadInteractionEdges();
  void LoadInitialPartitions();
  void SetupClientStubs();
  std::optional<std::shared_ptr<graph::Interaction>> GetInteraction();
  private:
  void MarkInteraction(std::shared_ptr<graph::Interaction> interaction);
  int self_rank_;
  int num_partitions_;
  int capacity_;
  int num_workers_;
  std::vector<std::vector<std::pair<int, int>>> matchings_;
  std::vector<std::vector<std::pair<int, int>>> plan_;
  std::vector<std::vector<std::pair<int, int>>> machine_state_;
  std::string server_address_;
  std::vector<std::thread> threads_;
  std::vector<std::vector<graph::InteractionEdges>> interaction_edges_;
  std::vector<graph::VertexPartition> partitions_first_half_;
  std::vector<graph::VertexPartition> partitions_second_half_;
  std::vector<std::pair<int, int>> super_partitiion_order_;
  std::vector<std::string> server_addresses_;
  std::vector<std::unique_ptr<graph::PartitionService::Stub>> client_stubs_;
  std::queue<std::shared_ptr<graph::Interaction>> interaction_queue_;
};
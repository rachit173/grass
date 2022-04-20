// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif



class DistributedBuffer {
 public:
  explicit DistributedBuffer(int self_rank, int num_partitions, int capacity, int num_workers);
  void GenerateMatchings(int l, int r, std::vector<std::vector<std::pair<int, int>>>& matchings);
  private:
  int self_rank_;
  int num_partitions_;
  int capacity_;
  int num_workers_;
  std::vector<std::vector<std::pair<int, int>>> matchings_;
};
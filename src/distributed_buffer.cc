#include "distributed_buffer.h"

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <vector>
#include <utility>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using graph::PartitionService;
using graph::PartitionRequest;
using graph::PartitionReply;
#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif
using namespace std;
typedef vector<vector<pair<int, int>>> vvii;

class PartitionServiceImpl final : public PartitionService::Service {
  private:
    DistributedBuffer* buffer_;
  public:
    explicit PartitionServiceImpl(DistributedBuffer* buffer) : buffer_(buffer) {}
    Status PartitionRequest(ServerContext* context, const PartitionRequest* request, PartitionReply* reply) {
      reply->set_partition_id(request->partition_id());
      return Status::OK;
    }
};

void DistributedBuffer::GenerateMatchings(int l, int r, vvii& matchings) {
  if ((r-l) == 2) {
    vector<pair<int, int>> tmp;
    tmp.push_back(make_pair(l, l+1));
    matchings.push_back(tmp);
    return;
  }
  int mid = l + (r-l)/2;
  vvii left_half;
  // left half
  GenerateMatchings(l, mid, left_half);
  vvii right_half;
  // right half
  GenerateMatchings(mid, r, right_half);
  // merge
  assert(left_half.size() == right_half.size());
  if (left_half.size() > 0) {
    for (int i = 0; i < (int)left_half.size(); i++) {
      auto tmp = left_half[i];
      for (auto& x: right_half[i]) {
        tmp.push_back(x);
      }
      matchings.push_back(tmp); 
    }
  }

  int n = (r-l);  
  // generate matchings using the bipartite graph
  for (int z = 0; z < n/2; z++) {
    vector<pair<int, int>> m;
    for (int i = 0; i < n/2; i++) {
      m.push_back(make_pair(l+i, mid+(i+z)%(n/2)));
    }
    matchings.push_back(m);
  }
}

void DistributedBuffer::StartBuffer() {
  std::this_thread::sleep_for(std::chrono::seconds(10));
}

void DistributedBuffer::StartServer() {
  PartitionServiceImpl service(this);
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  builder.SetMaxReceiveMessageSize(1 * 1024 * 1024 * 1024 + 1); // 1GB
  builder.SetMaxMessageSize(1 * 1024 * 1024 * 1024 + 1); // 1GB  
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  threads_.push_back(std::thread([&]() {
    server->Wait();
  }));
}

DistributedBuffer::DistributedBuffer(int self_rank, int num_partitions, 
                                      int capacity, int num_workers, std::string server_address): 
                                      self_rank_(self_rank),
                                      num_partitions_(num_partitions),
                                      capacity_(capacity),
                                      num_workers_(num_workers),
                                      server_address_(server_address) {
  // std::string filename = "/mnt/Work/grass/configs/planned.txt"
  // std::ifstream ss;
  // if (!ss.is_open()) {
  //   std::cerr << "Error opening file: " << filename << std::endl;
  //   exit(1);
  // }
  int n = num_workers * 2;
  matchings_.clear();
  GenerateMatchings(0, n, matchings_);

  // Start the server.
  StartServer();

  // Buffer starts working with self_rank_
  StartBuffer();
}
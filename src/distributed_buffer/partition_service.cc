#include "distributed_buffer.h"

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <vector>
#include <utility>
#include <fstream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using graph::VertexPartition;
using graph::InteractionEdges;
using graph::Edge;
using graph::Vertex;
using graph::PartitionService;
using graph::PartitionRequest;
using graph::PartitionResponse;
using graph::PingRequest;
using graph::PingResponse;
#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif
using namespace std;

// ########################### Partition Service ##############################
class PartitionServiceImpl final : public PartitionService::Service {
  private:
    DistributedBuffer* buffer_;
  public:
    explicit PartitionServiceImpl(DistributedBuffer* buffer) : buffer_(buffer) {}
    Status Ping(ServerContext* context, const PingRequest* request, PingResponse* response) {
      return Status::OK;
    }
    
    Status GetPartition(ServerContext* context, const PartitionRequest* request, PartitionResponse* response) {
      int super_partition_id = request->super_partition_id();
      std::cout << "GetPartition called requesting for super partition " << super_partition_id << std::endl;
      graph::VertexPartition* partition = buffer_->SendPartition(super_partition_id);
      response->set_super_partition_id(super_partition_id);
      response->mutable_partition()->set_partition_id(partition->partition_id());
      response->mutable_partition()->mutable_vertices()->CopyFrom(partition->vertices());
      
      
      delete partition;
      std::cout << "GetPartition returning partition" << response->partition().partition_id() << " for super partition " << super_partition_id << std::endl;

      buffer_->notifyEpochComplete();
      return Status::OK;
    }
};

void DistributedBuffer::StartBuffer() {
  std::cout << "Rank: " << self_rank_ << std::endl;
  std::cout << "Starting the distributed buffer for rank " << self_rank_ << std::endl;
  // Ping all other servers
  for (int r = 0; r < num_workers_; r++) {
    if (r == self_rank_) continue;
    grpc::ClientContext context;
    PingResponse response;
    PingRequest request;
    Status status = client_stubs_[r]->Ping(&context, request, &response);
    std::cout << "Ping: rank: " << r << ", address: " << server_addresses_[r] << ", status ok? " << status.ok() << std::endl;
  }
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
  server->Wait();  
}

void DistributedBuffer::SetupClientStubs() {
  // setup client stubs
  for (int i = 0; i < num_workers_; i++) {
    grpc::ChannelArguments ch_args;
    ch_args.SetMaxReceiveMessageSize(-1);
    std::string target_str = server_addresses_[i];
    auto channel = grpc::CreateCustomChannel(target_str, grpc::InsecureChannelCredentials(), ch_args);
    client_stubs_.push_back(PartitionService::NewStub(channel));
  }
}

void DistributedBuffer::notifyEpochComplete() {
  std::unique_lock<std::mutex> lock(mutex_epoch_completion_);
  cv_epoch_completion_.notify_one();
}

void DistributedBuffer::waitForEpochCompletion() {
  while(!done_partitions_.empty()) {
    std::unique_lock<std::mutex> lock(mutex_epoch_completion_);
    cv_epoch_completion_.wait(lock);
  }
}

// ############################################################################
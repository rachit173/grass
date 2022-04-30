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
using graph::PartitionReply;
#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif
using namespace std;

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

void DistributedBuffer::StartBuffer() {
  std::cout << "Starting the distributed buffer for rank " << self_rank_ << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // Ping all other servers.
  for (int r = 0; r < num_workers_; r++) {
    if (r == self_rank_) continue;
    grpc::ClientContext context;
    PartitionReply reply;
    PartitionRequest request;
    request.set_partition_id(0);
    Status status = client_stubs_[r]->GetPartition(&context, request, &reply);
    std::cout << "Ping: rank: " << r << ", address: " << server_addresses_[r] << ", status: " << status.ok() << std::endl;
  }                                                                                                                                     

  // Prepare the interaction queue.

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

void DistributedBuffer::MarkInteraction(WorkUnit interaction) {
  
}

// TODO: Remove this eventually. It's just for testing.
void DistributedBuffer::ProduceInteractions() {
  for(int i = 0; i < num_partitions_; i++) {
    for(int j = 0; j < num_partitions_; j++) {
      WorkUnit interaction(vertex_partitions_[i], vertex_partitions_[j], &interaction_edges_[i][j]);
      interaction_queue_.push(interaction);
    }
  }
}

// @TODO: change shared ptr for interaction to unique ptr.
std::optional<WorkUnit> DistributedBuffer::GetWorkUnit() {
  if (interaction_queue_.empty()) {
    return std::nullopt;
  }
  auto interaction = interaction_queue_.front();
  interaction_queue_.pop();
  MarkInteraction(interaction);
  return interaction;
}

DistributedBuffer::DistributedBuffer(DistributedBufferConfig config) {  
  self_rank_ = config.self_rank;
  num_partitions_ = config.num_partitions;
  capacity_ = config.capacity;
  num_workers_ = config.num_workers;
  server_address_ = config.server_addresses[self_rank_];
  server_addresses_ = config.server_addresses;
  interaction_edges_ = std::vector<std::vector<graph::InteractionEdges>>(num_partitions_, std::vector<graph::InteractionEdges>(num_partitions_));
  partitions_first_half_ = std::vector<graph::VertexPartition*>(capacity_/2, nullptr);
  partitions_second_half_ = std::vector<graph::VertexPartition*>(capacity_/2, nullptr);
  int n = num_workers_ * 2;
  matchings_.clear();
  GenerateMatchings(0, n, matchings_);
  GeneratePlan(matchings_, plan_, machine_state_);
  

  // Start the server.
  threads_.push_back(std::thread([&](){
    StartServer();
  }));

  // @TODO
  // Load the interaction edges.
  // Currently, all the interaction edges are loaded. 
  // Before loading interaction edges,  
  // GeneratePlan should be called.
  // LoadInteractionEdges();

  // @TODO
  // Load the initial partitions.
  // LoadInitialPartitions();

  // Setup client stubs to other servers. 
  SetupClientStubs();

  // Buffer starts working with self_rank_
  threads_.push_back(std::thread([&](){
    StartBuffer();
  }));
}
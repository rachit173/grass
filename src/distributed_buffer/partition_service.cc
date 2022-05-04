#include "distributed_buffer.h"

using graph::PartitionService;
using namespace std;

class PartitionServiceImpl final : public PartitionService::Service {
  private:
    DistributedBuffer* buffer_;
  public:
    explicit PartitionServiceImpl(DistributedBuffer* buffer) : buffer_(buffer) {}
    grpc::Status Ping(grpc::ServerContext* context, const graph::PingRequest* request, graph::PingResponse* response) {
      return grpc::Status::OK;
    }
    
    grpc::Status GetPartition(grpc::ServerContext* context, const graph::PartitionRequest* request, graph::PartitionResponse* response) {
      int super_partition_id = request->super_partition_id();
      spdlog::debug("[gRPC] GetPartition: Providing Super Partition: {}", super_partition_id);
      graph::VertexPartition* partition = buffer_->SendPartition(super_partition_id);
      response->set_super_partition_id(super_partition_id);
      response->mutable_partition()->set_partition_id(partition->partition_id());
      response->mutable_partition()->mutable_vertices()->CopyFrom(partition->vertices());
      
      delete partition;
      spdlog::debug("[gRPC] GetPartition: Partition {} for Super Partition {} sent", response->partition().partition_id(), super_partition_id);

      buffer_->NotifyPartitionSent(); // Notify the main thread that the partition has been sent
      return grpc::Status::OK;
    }
};

// Send partition from hashmap to other server.
graph::VertexPartition* DistributedBuffer::SendPartition(int super_partition_id) {
    graph::VertexPartition* partition;
    while(true) {
        std::unique_lock<std::mutex> lock(mutex_send_);
        for(auto it: done_partitions_) {
            int partition_id = it.first;
            if(BelongsToSuperPartition(partition_id, super_partition_id)) {
                partition = it.second;
                spdlog::debug("[gRPC] Sending Partition {} for Super Partition {}", partition->partition_id(), super_partition_id);
                done_partitions_.erase(it.first);
                return partition;
            }
        }
        spdlog::trace("[gRPC] Waiting for free Partition in Super Partition {}", super_partition_id);
        cv_send_.wait(lock);
    }
}

void DistributedBuffer::PingAll() {
  spdlog::info("Rank: {} - Starting Buffer", self_rank_);

  // Ping all other servers
  for (int r = 0; r < num_workers_; r++) {
    if (r == self_rank_) continue;
    grpc::ClientContext context;
    graph::PingResponse response;
    graph::PingRequest request;
    grpc::Status status = client_stubs_[r]->Ping(&context, request, &response);
    if (!status.ok()) {
      spdlog::error("[gRPC] Ping to server {} failed: {}", server_addresses_[r], status.error_message());
    } else {
      spdlog::info("[gRPC] Ping to server {} succeeded", server_addresses_[r]);
    }
  }
}

void DistributedBuffer::StartServer() {
  PartitionServiceImpl service(this);
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  grpc::ServerBuilder builder;

  builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.SetMaxReceiveMessageSize(1 * 1024 * 1024 * 1024 + 1); // 1GB
  builder.SetMaxMessageSize(1 * 1024 * 1024 * 1024 + 1); // 1GB  

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
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

void DistributedBuffer::NotifyPartitionSent() {
  std::unique_lock<std::mutex> lock(mutex_epoch_completion_);
  cv_epoch_completion_.notify_one();
}

// Wait for all partitions to be sent from the hashmap done_partitions_ (Sent by the gRPC thread)
void DistributedBuffer::WaitForEpochCompletion() {
  while(!done_partitions_.empty()) {
    std::unique_lock<std::mutex> lock(mutex_epoch_completion_);
    cv_epoch_completion_.wait(lock);
  }
}

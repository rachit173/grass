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
      // Requestor fill round must equal provider current round + 1
      int fill_round = request->incoming_round();
      if(fill_round != buffer_->GetCurrentRound() + 1) {
        spdlog::warn("Mismatch in fill round and current round. Fill round: {}, Current round: {}", fill_round, buffer_->GetCurrentRound());
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Wrong fill round");
      }
      spdlog::debug("[gRPC] GetPartition: Providing Super Partition: {}", super_partition_id);
      graph::VertexPartition* partition = buffer_->SendPartition(super_partition_id);
      response->set_super_partition_id(super_partition_id);
      response->mutable_partition()->set_partition_id(partition->partition_id());
      response->mutable_partition()->mutable_vertices()->CopyFrom(partition->vertices());
      buffer_->DeletePartition(partition);
      spdlog::debug("[gRPC] GetPartition: Partition {} for Super Partition {} sent", response->partition().partition_id(), super_partition_id);
      return grpc::Status::OK;
    }
};

void DistributedBuffer::DeletePartition(graph::VertexPartition* partition) {
  done_partitions_.erase(partition->partition_id());
  delete partition;
}

// Send partition from hashmap to other server.
graph::VertexPartition* DistributedBuffer::SendPartition(int super_partition_id) {
  graph::VertexPartition* partition = nullptr;
  while(true) {
    // Always release possible partitions before trying to send one
    for(auto vp: vertex_partitions_){
      if(vp == nullptr) continue;
      CheckAndReleaseOutgoingPartition(vp->partition_id());
    }
    
    std::unique_lock<std::mutex> lock(mutex_send_);
    for(auto it: done_partitions_) {
      int partition_id = it.first;
      if(!BelongsToSuperPartition(partition_id, super_partition_id)) continue;
      
      partition = it.second;
      spdlog::critical("[gRPC] Sending Partition {}", partition->partition_id());
      break;
    }

    if(partition != nullptr) {
      // Trigger round change if 1 superpartition sent in this 'round'. Only 1 round change possible at a time.
      current_round_partitions_sent_++;
      if(current_round_partitions_sent_ == (capacity_/2)) {
        current_round_partitions_sent_ = 0;
        current_round_++;
        spdlog::critical("[gRPC] Round incremented to {}", current_round_);
      }
      return partition;
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

// Wait for all partitions to be sent from the hashmap done_partitions_ (Sent by the gRPC thread)
void DistributedBuffer::WaitForEpochCompletion() {
  // IsEpochComplete --> interactions all done + all partitions sent to other servers except those in last round
  while(!IsEpochComplete()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

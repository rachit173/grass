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

void GenerateMatchings(int l, int r, vvii& matchings) {
  auto ringer = [](int x, int m) {
    return (x+m)%m;
  };
  if ((r-l) == 2) {
    vector<pair<int, int>> tmp;
    tmp.push_back(make_pair(l, l+1));
    matchings.push_back(tmp);
    return;
  }
  if (((r-l)/2)%2==1) { 
    // This implies that (r-l) is of the type 2*q = 2*(2u+1), so, q = 2u+1
    int q = (r-l)/2;
    int u = (q-1)/2;
    int m = l+q;
    for (int i = 0; i < q; i++) {
      vector<pair<int, int>> tmp;      
      tmp.push_back({l+i, m+i});
      for (int j = 1; j <= u; j++) {
        tmp.push_back({l+ringer(i+j, q), l+ringer(i-j, q)});
        tmp.push_back({m+ringer(i+j, q), m+ringer(i-j, q)});
      }
      matchings.push_back(tmp);
    }
    for (int j = 1; j < q; j++) {
      vector<pair<int, int>> tmp;
      for (int k = 0; k < q; k++) {
        tmp.push_back({l+k, m+ringer(k+j, q)});
      }
      matchings.push_back(tmp);
    }
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
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // Ping all other servers.
  for (int r = 0; r < num_workers_; r++) {
    if (r == self_rank_) continue;
    grpc::ClientContext context;
    PartitionReply reply;
    PartitionRequest request;
    request.set_partition_id(0);
    Status status = client_stubs_[self_rank_^1]->GetPartition(&context, request, &reply);
    std::cout << "Ping " << r << ": " << status.ok() << std::endl;
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

void DistributedBuffer::LoadInitialPartitions() {
  // load initial partitions
  int left_super_partition = (2*self_rank_);
  int right_super_partition = (2*self_rank_+1);
  int B = capacity_/2;
  for (int i = 0; i < capacity_ / 2; i++) {
    graph::VertexPartition part;
    part.set_partition_id(left_super_partition*B + i);
    partitions_first_half_.push_back(part);
  }
  for (int i = capacity_ / 2; i < capacity_; i++) {
    graph::VertexPartition part;
    part.set_partition_id(right_super_partition*B + i);
    partitions_second_half_.push_back(part);
  }
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

void DistributedBuffer::LoadInteractionEdges() {
  for (int i = 0; i < num_partitions_; i++) {
    std::vector<InteractionEdges> interaction_vect;
    for (int j = 0; j < num_partitions_; j++) {
      interaction_vect.push_back(InteractionEdges());
    }
    interaction_edges_.push_back(interaction_vect);
  }
}

void DistributedBuffer::MarkInteraction(std::shared_ptr<graph::Interaction> interaction) {
  
}

// @TODO: change shared ptr for interaction to unique ptr.
std::optional<std::shared_ptr<graph::Interaction>> DistributedBuffer::GetInteraction() {
  if (interaction_queue_.empty()) {
    return std::nullopt;
  }
  auto interaction = interaction_queue_.front();
  interaction_queue_.pop();
  MarkInteraction(interaction);
  return interaction;
}

void GeneratePlan(vector<vector<pair<int, int>>>& matchings, vector<vector<pair<int, int>>>& plan, vector<vector<pair<int, int>>>& machine_state) {
  // matchings -> for this machine a vector<pair> which contains the machine_id, superpartition_id
  int k = matchings[0].size();
  int n = 2*k;
  int rounds = matchings.size();
  plan.resize(rounds, vector<pair<int, int>>(k));
  machine_state.resize(rounds, vector<pair<int, int>>(k));
  int partition_machine[rounds][n];
  // Initialiazation for round 0 for all machines. 
  for (int w = 0; w < k; w++) {
    plan[0][w] = {w, -1};
  }
  for (int i = 0; i < k; i++) {
    auto e = matchings[0][i];
    partition_machine[0][e.first] = i;
    partition_machine[0][e.second] = i;
    machine_state[0][i] = e;
  }

  // Iteration for all other rounds.
  for (int i = 1; i < rounds; i++) {
    const auto current_matching = matchings[i-1];
    const auto next_matching = matchings[i];
    
    int current_map[n];
    for (auto e: current_matching) {
      current_map[e.first] = e.second;
      current_map[e.second] = e.first;
    }
    int next_map[n];
    for (auto e: next_matching) {
      next_map[e.first] = e.second;
      next_map[e.second] = e.first;      
    }

    vector<int> cycle_prev_p(n, -1);
    vector<int> visited(n, 0);
    // Generate the cycle
    for (int p = 0; p < n; p++) {
      if (visited[p]) continue;
      int x = 0; // edge from current or next matching
      int ptr = p;
      while (visited[ptr] != 1) {
        int prev_ptr = ptr;
        visited[ptr] = 1;
        if (x == 1) {
          ptr = next_map[ptr];
          cycle_prev_p[ptr] = prev_ptr;
        } else {
          ptr = current_map[ptr];
        }
        x ^= 1;
      }
    }
    
    for (int j = 0; j < n; j++) {
      if (cycle_prev_p[j] == -1) continue;
      int p1 = j;
      int p2 = cycle_prev_p[j];

      int machine_of_p1 = partition_machine[i-1][p1];
      int machine_of_p2 = partition_machine[i-1][p2];
      // machine_of_p1 requests machine_of_p2 for p2

      plan[i][machine_of_p1] = {machine_of_p2, p2}; 
      partition_machine[i][p1] = machine_of_p1;
      partition_machine[i][p2] = machine_of_p1;
      machine_state[i][machine_of_p1] = {p1, p2};
    }
  }
}

DistributedBuffer::DistributedBuffer(DistributedBufferConfig config) {
  // std::string filename = "/mnt/Work/grass/configs/planned.txt"
  // std::ifstream ss;
  // if (!ss.is_open()) {
  //   std::cerr << "Error opening file: " << filename << std::endl;
  //   exit(1);
  // }
  self_rank_ = config.self_rank;
  num_partitions_ = config.num_partitions;
  capacity_ = config.capacity;
  num_workers_ = config.num_workers;
  server_address_ = config.server_addresses[self_rank_];
  server_addresses_ = config.server_addresses;
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
  LoadInteractionEdges();

  // @TODO
  // Load the initial partitions.
  LoadInitialPartitions();

  // Setup client stubs to other servers. 
  SetupClientStubs();

  // Buffer starts working with self_rank_
  threads_.push_back(std::thread([&](){
    StartBuffer();
  }));
}
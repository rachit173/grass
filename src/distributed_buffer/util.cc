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
#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif
using namespace std;
typedef vector<vector<pair<int, int>>> vvii;

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

void GeneratePlan(vector<vector<pair<int, int>>>& matchings, vector<vector<pair<int, int>>>& plan, vector<vector<pair<int, int>>>& machine_state, vector<vector<int>>& partition_to_be_sent) {
  // matchings -> for this machine a vector<pair> which contains the machine_id, superpartition_id
  int k = matchings[0].size();
  int n = 2*k;
  int rounds = matchings.size();
  plan.resize(rounds, vector<pair<int, int>>(k));
  machine_state.resize(rounds, vector<pair<int, int>>(k));
  partition_to_be_sent.resize(rounds, vector<int>(k));

  int partition_machine[rounds][n];
  // Initialiazation for round 0 for all machines. 
  for (int w = 0; w < k; w++) {
    plan[0][w] = {w, -1};
    partition_to_be_sent[0][w] = -1;
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

      // Partition to be sent is the one that was present in previous cycle but is not in the current cycle.
      if(machine_state[i - 1][machine_of_p1].first != p1){ // not checking for p2 as p2 is not on the machine.
        partition_to_be_sent[i - 1][machine_of_p1] = machine_state[i - 1][machine_of_p1].first;
      } else {
        partition_to_be_sent[i - 1][machine_of_p1] = machine_state[i - 1][machine_of_p1].second;
      }
    }
  }
}

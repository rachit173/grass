#include "distributed_buffer.h"

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <vector>
#include <utility>

#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif
using namespace std;
typedef vector<vector<pair<int, int>>> vvii;

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
    for (int i = 0; i < left_half.size(); i++) {
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

DistributedBuffer::DistributedBuffer(int self_rank, int num_partitions, 
                                      int capacity, int num_workers): 
                                      self_rank_(self_rank),
                                      num_partitions_(num_partitions),
                                      capacity_(capacity),
                                      num_workers_(num_workers) {
  // std::string filename = "/mnt/Work/grass/configs/planned.txt"
  // std::ifstream ss;
  // if (!ss.is_open()) {
  //   std::cerr << "Error opening file: " << filename << std::endl;
  //   exit(1);
  // }
  int n = num_workers * 2;
  matchings_.clear();
  GenerateMatchings(0, n, matchings_);
}
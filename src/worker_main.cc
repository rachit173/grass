#include <iostream>
#include <fstream>

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif

using graph::Vertex;
using graph::Edge;
using graph::VertexPartition;

void PageRank(VertexPartition& partition, int num_iters=5) {

}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <graph_data>" << std::endl;
    return 1;
  }
  std::string filename = argv[1];
  std::ifstream graphin;
  graphin.open(filename);
  if (!graphin.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return 1;
  }
  int num_vertices;
  graphin >> num_vertices;
  VertexPartition partition;
  for (int i = 0; i < num_vertices; i++) {
    auto vertex = partition.add_vertices();
    vertex->set_id(i);
    vertex->mutable_state()->set_old_result(0);
  }
  int src, dst;
  while (graphin >> src >> dst) {
    auto edge = partition.add_edges();
    edge->set_src(src);
    edge->set_dst(dst);
  }
  std::cout << "Num vertices: " << partition.vertices().size() << ", Num edges:  " << partition.edges().size() << std::endl;
  PageRank(partition, 5);
  return 0;
}
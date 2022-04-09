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
using graph::Interaction;

void InitVertexState(VertexPartition& partition) {

}

void ProcessInteraction(Interaction& interaction) {

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
  std::vector<Edge> edges;
  while (graphin >> src >> dst) {
    edges.push_back(Edge(src, dst));
  }
  std::cout << "Num vertices: " << partition.vertices().size() << ", Num edges:  " << partition.edges().size() << std::endl;
  std::cout << "Num edges: " << edges.size() << std::endl;
  const int num_partitions = 2;
  VertexPartition vertex_partitions[num_partitions];
  InteractionEdges interaction_edges[num_partitions][num_partitions];
  // TODO: make partitions 
  // This should populate vertex_partitions
  // and interaction edges
  MakePartitions(VertexPartition, edges, num_partitions, vertex_partitions, interaction_edges);
  // TODO

  for (int i = 0; i < num_partitions; i++) {
    InitVertexState(vertex_partitions[i]);
  }
  const int num_pagerank_iters = 1;
  for (int i = 0; i < num_pagerank_iters; i++) {
    // Actual Interactions
    for (int j = 0; j < num_partitions; j++) {
      for (int k = 0; k < num_partitions; k++) {
        Interaction interaction(vertex_partitions[i], vertex_partitions[j], interaction_edges[i][j]);
        ProcessInteraction(interaction);
      }
    }

  }
  return 0;
}
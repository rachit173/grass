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

#ifdef BAZEL_BUILD
// #include "examples/protos/helloworld.grpc.pb.h"
#else
#include "protos/graph.grpc.pb.h"
#endif

using graph::Vertex;
using graph::Edge;
using graph::VertexPartition;
using graph::Interaction;
using graph::InteractionEdges;
using graph::Int32;
using graph::Double;
using graph::String;
using graph::Bool;


//////////////////////////////////////////////////////////////////////////////////////////////////////
int num_vertices, num_edges;
std::vector<Edge> edges;

const double ACCUMULATOR_INIT_VAL = 0.0;

void init(Vertex& vertex) {
  // set init state for PageRank
  Double init_val;
  init_val.set_value(1.0);

  Double init_accum_val;
  init_accum_val.set_value(ACCUMULATOR_INIT_VAL);

  vertex.mutable_state()->mutable_result()->PackFrom(init_val);
  vertex.mutable_state()->mutable_accumulator()->PackFrom(init_accum_val);
}

// PageRank accumulator
void gather(Vertex& src, Vertex& dst, const Edge& edge) {
  Double prev_page_rank;
  if(!src.state().result().Is<Double>()) {
    std::cerr << "Error: Failed to unpack PageRank value" << std::endl;
    exit(1);
  }
  src.state().result().UnpackTo(&prev_page_rank);

  int64_t out_degree = src.degree().out_degree();
  double contribution = (prev_page_rank.value() / (double) out_degree); 

  Double acc;
  if(!src.state().accumulator().Is<Double>()) {
    std::cerr << "Error: Failed to unpack PageRank accumulator" << std::endl;
    exit(1);
  }
  dst.state().accumulator().UnpackTo(&acc);
  // gather contribution by adding to accumulator
  acc.set_value(acc.value() + contribution);
  dst.mutable_state()->mutable_accumulator()->PackFrom(acc);
}

void apply(Vertex& vertex) {
  Double acc;
  vertex.state().accumulator().UnpackTo(&acc);
  
  Double new_result;
  new_result.set_value((1.0 - 0.85) + 0.85 * acc.value());
  vertex.mutable_state()->mutable_result()->PackFrom(new_result);

  Double init_accum_val;
  init_accum_val.set_value(ACCUMULATOR_INIT_VAL);
  vertex.mutable_state()->mutable_accumulator()->PackFrom(init_accum_val);
}

// ################################################################################################ //

int64_t PARTITION_SIZE = 0.0;

void InitVertexState(VertexPartition& partition) {
  for (int i = 0; i < partition.vertices().size(); i++) {
    Vertex* vertex = partition.mutable_vertices(i);
    init(*vertex);
  }
}

void ProcessInteraction(VertexPartition *src_partition, VertexPartition *dst_partition, const InteractionEdges *directed_edges) {
  std::cout << "Processing interaction" << std::endl;
  int64_t num_interaction_edges = directed_edges->edges().size();
  for(int64_t i = 0; i < num_interaction_edges; i++) {
    const Edge *edge = &directed_edges->edges(i);
    
    // S[u] --> interaction.src[edge.src % partition_size]
    // S[v] --> interaction.dst[edge.dst % partition_size]
    // accumulator_add(S[v], S[u], edge)

    int src_vertex_id = edge->src(), dst_vertex_id = edge->dst();
    Vertex* src = src_partition->mutable_vertices(src_vertex_id % PARTITION_SIZE);
    Vertex* dst = dst_partition->mutable_vertices(dst_vertex_id % PARTITION_SIZE);

    gather(*src, *dst, *edge);
  }
}

void ApplyPhase(VertexPartition& partition) {
  for (int i = 0; i < partition.vertices().size(); i++) {
    Vertex* vertex = partition.mutable_vertices(i);
    apply(*vertex);
  }
}

void MakePartitions(int num_partitions, std::vector<VertexPartition>& vertex_partitions, std::vector<std::vector<InteractionEdges>>& interaction_edges){
  int partition_size = (int)(ceil((double) num_vertices / (double)num_partitions));
  PARTITION_SIZE = partition_size;
  std::cout << "Making Partitions of size: " << partition_size << std::endl;
  //1.  Divide number of vertices into partitions
  for(int i = 0; i < num_partitions; i++) {
    int partition_start = i * partition_size, partition_end = std::min((i+1) * partition_size, num_vertices);
    
    // Default initialization for Vertex
    for(int j = partition_start; j < partition_end; j++){ // j --> vertex id
      Vertex* vertex = vertex_partitions[i].add_vertices();
      vertex->set_id(j);
      vertex->mutable_degree()->set_out_degree(0);
      vertex->mutable_degree()->set_in_degree(0);
    }
  }
  
  //2. Find edges between partitions
  int64_t num_edges = edges.size();
  for(int64_t i = 0; i < num_edges; i++) {
    int src = edges[i].src(), dst = edges[i].dst();
    int src_partition = src / partition_size, dst_partition = dst / partition_size;
    Edge* edge = interaction_edges[src_partition][dst_partition].add_edges();
    *edge = edges[i];
    Vertex *src_vertex = vertex_partitions[src_partition].mutable_vertices(src % partition_size);
    Vertex *dst_vertex = vertex_partitions[dst_partition].mutable_vertices(dst % partition_size);
    src_vertex->mutable_degree()->set_out_degree(src_vertex->degree().out_degree() + 1);
    dst_vertex->mutable_degree()->set_in_degree(dst_vertex->degree().in_degree() + 1);
  }
}

// #########################################################################################################

void printPartition(VertexPartition &partition) {
  int num_vertices = partition.vertices().size();
  for(int i = 0; i < num_vertices; i++) {
    Vertex vertex = partition.vertices(i);
    Double result;
    vertex.state().result().UnpackTo(&result);
    printf("vertex id: %ld, value: %f, out_degree: %d, in_degree: %d\n", vertex.id(), result.value(),
            vertex.degree().out_degree(), vertex.degree().in_degree());
  }
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
  graphin >> num_vertices;
  VertexPartition partition;
  int src, dst;
  Edge edge;
  while (graphin >> src >> dst) {
    edge.set_src(src);
    edge.set_dst(dst);
    edges.push_back(edge);
  }
  std::cout << "Num edges: " << edges.size() << std::endl;
  const int num_partitions = 2;
  std::vector<VertexPartition> vertex_partitions(num_partitions);
  std::vector<std::vector<InteractionEdges>> interaction_edges(num_partitions, std::vector<InteractionEdges>(num_partitions));
  
  // TODO: make partitions 
  // This should populate vertex_partitions
  // and interaction edges
  MakePartitions(num_partitions, vertex_partitions, interaction_edges);
  for(int i = 0; i < num_partitions; i++){
    std::cout << "Partition " << i << std::endl;
    std::cout << "Num vertices: " << vertex_partitions[i].vertices().size() << std::endl;

    for(int j = 0; j < num_partitions; j++){
      std::cout << "Interaction edges from " << i << " to " << j << ": " << interaction_edges[i][j].edges_size() << std::endl;
    }
  }
  // TODO
  // Initialize partition state
  for (int i = 0; i < num_partitions; i++) {
    InitVertexState(vertex_partitions[i]);
  }

  const int num_pagerank_iters = 2;
  for (int iter = 0; iter < num_pagerank_iters; iter++) {
    
    // Gather Phase
    for (int i = 0; i < num_partitions; i++) {
      for (int j = 0; j < num_partitions; j++) {
        VertexPartition* src =  &vertex_partitions[i];
        VertexPartition* dst = &vertex_partitions[j];
        InteractionEdges* edges = &interaction_edges[i][j];
        ProcessInteraction(src, dst, edges);
      }
    }

    // Apply Phase
    for(int i = 0; i < num_partitions; i++) {
      ApplyPhase(vertex_partitions[i]);
    }

    // Scatter phase
  }


  // Write PageRank to file
  std::ofstream outfile;
  outfile.open("/mnt/Work/grass/resources/graphs/calculated_pagerank.txt");
  for (int i = 0; i < num_partitions; i++) {
    for (int j = 0; j < vertex_partitions[i].vertices().size(); j++) {
      Vertex* vertex = vertex_partitions[i].mutable_vertices(j);
      Double result;
      vertex->state().result().UnpackTo(&result);
      outfile << vertex->id() << " " << result.value() << std::endl;
    }
  }

  outfile.close();
  return 0;
}
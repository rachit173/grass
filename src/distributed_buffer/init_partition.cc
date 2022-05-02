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


int DistributedBuffer::LoadInteractionEdges(std::string& graph_file, bool weighted_edges, std::vector<graph::Edge*>& edges) {
  // Open file
  std::ifstream graph_file_stream(graph_file);
  int num_vertices;
  if (!graph_file_stream.is_open()) {
      std::cerr << "Error opening file: " << graph_file << ". Exiting..." << std::endl;
      exit(1);
  }

  graph_file_stream >> num_vertices;
  partition_size_ = (int)(ceil((double) num_vertices / (double)num_partitions_));
  std::cout << "Partition size: " << partition_size_ << std::endl;
  int src, dst;
  double weight = 1.0;
  
  while (graph_file_stream >> src >> dst) {
    // Read and store edge
    graph::Edge* edge = new graph::Edge();
    edge->set_src(src);
    edge->set_dst(dst);
    if(weighted_edges) {
        graph_file_stream >> weight;
    }
    edge->set_weight(weight);
    // std::cout << "Edge: (" << edge.src() << ", " << edge.dst() << ") --> wt =" << edge.weight() << std::endl;
    edges.push_back(edge);

    // Put edges in Interaction edge buckets
    int src_partition = src / partition_size_, dst_partition = dst / partition_size_;
    graph::Edge *interEdge = interaction_edges_[src_partition][dst_partition].add_edges();
    *interEdge = *edge;
  }
  graph_file_stream.close();
  return num_vertices;
}



graph::VertexPartition* DistributedBuffer::InitPartition(int partition_id, int partition_start, int partition_end) {
  graph::VertexPartition* partition = new graph::VertexPartition();
  partition->set_partition_id(partition_id);
  for(int j = partition_start; j < partition_end; j++){ // j --> vertex id
      graph::Vertex* vertex = partition->add_vertices();
      // Default initialization for Vertex
      vertex->set_id(j);
      vertex->mutable_degree()->set_out_degree(0);
      vertex->mutable_degree()->set_in_degree(0);
  }
  return partition;
}

void DistributedBuffer::InitSuperPartition(std::vector<graph::VertexPartition*>& super_partition, int super_partition_id) {
  int B = capacity_/2;
  for(int i = 0; i < B; i++) {
    int partition_id = super_partition_id * B + i;
    int partition_start = partition_id * partition_size_, partition_end = (partition_id + 1) * partition_size_; // TODO: Put cap on size at num_vertices_?
    super_partition[i] = InitPartition(partition_id, partition_start, partition_end); // [partition_start, partition_end)
    buffer_size_++; // Track buffer size
  }
}

void DistributedBuffer::LoadInitialPartitions() {
  int left_super_partition = (2*self_rank_); // machine_state_[0][self_rank_].first;
  int right_super_partition = (2*self_rank_+1); // machine_state_[0][self_rank_].second;
  int B = capacity_/2;

  InitSuperPartition(partitions_first_half_, left_super_partition);
  InitSuperPartition(partitions_second_half_, right_super_partition);
  
  // whole view of vertex partitions in buffer
  vertex_partitions_ = std::vector<graph::VertexPartition*>(capacity_, nullptr);
  for(int i = 0; i < B; i++) {
    vertex_partitions_[i] = partitions_first_half_[i];
    vertex_partitions_[i + B] = partitions_second_half_[i];
  }

  // std::cout << "Initial Partitions loaded." << std::endl;
  // for(int i = 0; i < capacity_; i++) {
  //   std::cout << "Vertex partition id: " << vertex_partitions_[i]->partition_id() << std::endl;
  // }

  // Produce initial interactions
  ProduceInteractions();

  // Start thread for filling the interactions queue
  threads_.push_back(std::thread([&](){
    FillPartitions();
  }));
}

void DistributedBuffer::ProduceInteractions() {
  for(int i = 0; i < capacity_; i++) {
    for(int j = 0; j < capacity_; j++) {
      WorkUnit interaction(vertex_partitions_[i], vertex_partitions_[j], &interaction_edges_[i][j]);
      // std::cout << "Inserting interaction: (" << vertex_partitions_[i]->partition_id() << ", " << vertex_partitions_[j]->partition_id() << ") to queue." << std::endl;
      interaction_queue_.push(interaction);
    }
  }
}

std::vector<graph::VertexPartition*>& DistributedBuffer::get_partitions() {
  return vertex_partitions_;
}
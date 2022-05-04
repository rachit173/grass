#include "distributed_buffer.h"

using namespace std;

void DistributedBuffer::LoadInteractionEdges(std::string& graph_file, bool weighted_edges) {
  // Open file
  std::ifstream graph_file_stream(graph_file);
  if (!graph_file_stream.is_open()) {
    spdlog::error("Could not open file {}. Exiting...", graph_file);
    exit(1);
  }

  interaction_edges_ = std::vector<std::vector<graph::InteractionEdges>>(num_partitions_, std::vector<graph::InteractionEdges>(num_partitions_));
  
  graph_file_stream >> num_vertices_;
  partition_size_ = (int)(ceil((double) num_vertices_ / (double)num_partitions_));
  spdlog::info("Partition size: {}", partition_size_);
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

    // Put edges in Interaction edge buckets
    int src_partition = src / partition_size_, dst_partition = dst / partition_size_;
    graph::Edge *interEdge = interaction_edges_[src_partition][dst_partition].add_edges();
    *interEdge = *edge;
    num_edges_++;
  }
  graph_file_stream.close();
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
    int partition_start = partition_id * partition_size_, partition_end = std::min(num_vertices_, (int64_t)( (partition_id + 1) * partition_size_) ); // Put cap on end at num_vertices_
    super_partition[i] = InitPartition(partition_id, partition_start, partition_end); // [partition_start, partition_end)
    buffer_size_++; // Track buffer size
  }
}

void DistributedBuffer::LoadInitialPartitions() {
  int left_super_partition = (2*self_rank_); // machine_state_[0][self_rank_].first;
  int right_super_partition = (2*self_rank_+1); // machine_state_[0][self_rank_].second;
  int B = capacity_/2;
  partitions_first_half_ = std::vector<graph::VertexPartition*>(capacity_/2, nullptr);
  partitions_second_half_ = std::vector<graph::VertexPartition*>(capacity_/2, nullptr);

  InitSuperPartition(partitions_first_half_, left_super_partition);
  InitSuperPartition(partitions_second_half_, right_super_partition);
  
  // whole view of vertex partitions in buffer
  vertex_partitions_ = std::vector<graph::VertexPartition*>(capacity_, nullptr);
  for(int i = 0; i < B; i++) {
    vertex_partitions_[i] = partitions_first_half_[i];
    vertex_partitions_[i + B] = partitions_second_half_[i];
  }
}

void DistributedBuffer::ProduceInteractions() {
  std::unique_lock buffer_lock(buffer_mutex_);
  for(int i = 0; i < capacity_; i++) {
    for(int j = 0; j < capacity_; j++) {
      graph::VertexPartition* src = vertex_partitions_[i];
      graph::VertexPartition* dst = vertex_partitions_[j];
      graph::InteractionEdges* interEdges = &interaction_edges_[src->partition_id()][dst->partition_id()];
      WorkUnit interaction(src, dst, interEdges);
      interaction_queue_.push(interaction);
    }
  }
}
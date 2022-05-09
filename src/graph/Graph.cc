#include "Graph.h"

template <typename R, typename A>
Graph<R, A>::Graph(DistributedBuffer* buffer, std::string input_file, bool weighted_edges)
: input_file_(input_file),
  weighted_edges_(weighted_edges),
  buffer_(buffer) {

  auto init_interactions_func = [this]() {
    this->LoadInteractions();
  };
  
  auto init_partition_func = [this](partition::Partition *partition, int partition_start, int partition_end) {
    this->InitPartition(partition, partition_start, partition_end);
  };

  buffer_->Load(init_interactions_func, init_partition_func);
  buffer_->Start();
  buffer_->GetVertexPartitions(vertex_partitions_);
}

template <typename R, typename A>
void Graph<R, A>::InitPartition(partition::Partition *partition, int partition_start, int partition_end) {
  int64_t partition_id = partition->partition_id();
  std::vector<int> partition_vertices = buffer_->GetPartitionVertices(partition_id);

  graph::VertexPartition* vertex_partition = partition->mutable_vertex_partition();
  for (auto v : partition_vertices) {
      graph::Vertex* vertex = vertex_partition->add_vertices();
      // Default initialization for Vertex
      vertex->set_id(v);
      vertex->mutable_degree()->set_out_degree(0);
      vertex->mutable_degree()->set_in_degree(0);
  }
}

template <typename R, typename A>
void Graph<R, A>::LoadInteractions() {
  int64_t num_vertices, num_edges, partition_size;

  int num_partitions = buffer_->GetNumPartitions();
  std::vector<std::vector<partition::Interaction>> &interactions = buffer_->GetInteractions();

  if (!interactions.empty()) {
    spdlog::debug("Interactions are already loaded");
    return;
  }

  interactions = std::vector<std::vector<partition::Interaction>>(num_partitions, std::vector<partition::Interaction>(num_partitions));

  std::ifstream graph_file_stream(input_file_);
  if (!graph_file_stream.is_open()) {
    spdlog::error("Could not open file {}. Exiting...", input_file_);
    exit(1);
  }
  
  graph_file_stream >> num_vertices;
  partition_size = (int)(ceil((double) num_vertices / (double)num_partitions));
  spdlog::info("Partition size: {}", partition_size);
  int src, dst;
  double weight = 1.0;
  
  graph::Edge* edge = new graph::Edge();
  while (graph_file_stream >> src >> dst) {
    // Read and store edge
    edge->set_src(src);
    edge->set_dst(dst);
    if(weighted_edges_) {
        graph_file_stream >> weight;
    }
    edge->set_weight(weight);

    // Put edges in Interaction edge buckets
    int src_partition = buffer_->GetPartitionHash(src);
    int dst_partition = buffer_->GetPartitionHash(dst);

    graph::InteractionEdges *graph_interaction_edges = interactions[src_partition][dst_partition].mutable_interaction_edges();
    graph::Edge *interEdge = graph_interaction_edges->add_edges();
    *interEdge = *edge;
    num_edges++;
  }

  std::stringstream ss;
  for(int i = 0; i < num_partitions; i++) {
    for(int j = 0; j < num_partitions; j++) {
      graph::InteractionEdges graph_interaction_edges = interactions[i][j].interaction_edges();
      ss << graph_interaction_edges.edges_size() << ",";
    }
    ss << "\n";
  }
  spdlog::trace("Edge partition sizes: \n{}", ss.str());

  graph_file_stream.close();

  buffer_->SetNumEdges(num_edges);
  buffer_->SetNumVertices(num_vertices);
  buffer_->SetPartitionSize(partition_size);
}

template <typename R, typename A>
void Graph<R, A>::initialize() {
    buffer_->GetVertexPartitions(vertex_partitions_);
    int num_local_partitions = vertex_partitions_.size();
    for (int i = 0; i < num_local_partitions; i++) {
        for (int j = 0; j < vertex_partitions_[i]->vertices().size(); j++) {
            graph::Vertex* vertex = vertex_partitions_[i]->mutable_vertices(j);
            Vertex<R, A> v(vertex);
            init_func_(v);
        }
    }
}

template <typename R, typename A>
void Graph<R, A>::startProcessing(const int &num_iters) {
    for (int iter = 0; iter < num_iters; iter++) {
        spdlog::info("Iteration: {}", iter);
        buffer_->InitEpoch();
        // Gather Phase
        while(true){
            std::optional<WorkUnit> opt_interaction = buffer_->GetWorkUnit();
            if(opt_interaction == std::nullopt){
                spdlog::debug("Gather phase complete.");
                break;
            }
            
            WorkUnit interaction = *opt_interaction;
            partition::Partition *src_partition = interaction.src();
            partition::Partition *dst_partition = interaction.dst();
            partition::Interaction *interaction_edges = interaction.edges();
            spdlog::trace("Processing interaction: src: {}, dst: {}", src_partition->partition_id(), dst_partition->partition_id());

            graph::VertexPartition* src = src_partition->mutable_vertex_partition();
            graph::VertexPartition* dst = dst_partition->mutable_vertex_partition();
            graph::InteractionEdges* edges = interaction_edges->mutable_interaction_edges();
            processInteraction(src, dst, edges);
            buffer_->MarkInteraction(interaction);
        }

        // Apply Phase
        buffer_->GetVertexPartitions(vertex_partitions_);
        int num_local_partitions = vertex_partitions_.size();
        for(int i = 0; i < num_local_partitions; i++) {
            applyPhase(*vertex_partitions_[i]);
        }

        buffer_->WaitForEpochCompletion();
        // Scatter phase
    }
}

template <typename R, typename A>
void Graph<R, A>::collectResults() {
    vertices_.clear();
    buffer_->GetVertexPartitions(vertex_partitions_);
    int num_local_partitions = vertex_partitions_.size();
    for (int i = 0; i < num_local_partitions; i++) {
        for (int j = 0; j < vertex_partitions_[i]->vertices().size(); j++) {
            graph::Vertex* vertex = vertex_partitions_[i]->mutable_vertices(j);
            vertices_.push_back(Vertex<R, A>(vertex));
        }
    }
}

template <typename R, typename A>
std::vector< Vertex<R, A> >& Graph<R, A>::get_vertices() {
    return vertices_;
}

template <typename R, typename A>
void Graph<R, A>::processInteraction(graph::VertexPartition *src_partition, graph::VertexPartition *dst_partition, const graph::InteractionEdges *directed_edges) {
    int64_t num_interaction_edges = directed_edges->edges().size();
    // std::vector<Vertex<R,A>> src_vertices, dst_vertices;
    std::unordered_map<int64_t, Vertex<R, A>> src_vertices, dst_vertices;
    // Expose partition size from buffer
    int partition_size = buffer_->GetPartitionSize();
    for (int64_t i = 0; i < src_partition->vertices().size(); i++) {
        graph::Vertex* vertex = src_partition->mutable_vertices(i);
        src_vertices[vertex->id()] = Vertex<R,A>(vertex);
    }
    for (int64_t i = 0; i < dst_partition->vertices().size(); i++) {
        graph::Vertex* vertex = dst_partition->mutable_vertices(i);
        dst_vertices[vertex->id()] = Vertex<R,A>(vertex);
    }

    Edge edge_obj;
    for(int64_t i = 0; i < num_interaction_edges; i++) {
        const graph::Edge *edge = &directed_edges->edges(i);
        
        // S[u] --> interaction.src[edge.src % partition_size]
        // S[v] --> interaction.dst[edge.dst % partition_size]
        // accumulator_add(S[v], S[u], edge)

        int src_vertex_id = edge->src(), dst_vertex_id = edge->dst();
        edge_obj.set_edge(edge);
        gather_func_(src_vertices[src_vertex_id], dst_vertices[dst_vertex_id], edge_obj);
    }
}

template <typename R, typename A>
void Graph<R, A>::applyPhase(graph::VertexPartition& partition) {
    for (int i = 0; i < partition.vertices().size(); i++) {
        graph::Vertex* vertex = partition.mutable_vertices(i);
        Vertex<R, A> v(vertex);
        apply_func_(v);
    }
}

template <typename R, typename A>
void Graph<R, A>::set_init_func(init_func_t init_func) {
    init_func_ = init_func;
}

template <typename R, typename A>
void Graph<R, A>::set_gather_func(gather_func_t gather_func) {
    gather_func_ = gather_func;
}

template <typename R, typename A>
void Graph<R, A>::set_apply_func(apply_func_t apply_func) {
    apply_func_ = apply_func;
}

template class Graph<double, double>;
// template class Graph<Int32, Int32>;
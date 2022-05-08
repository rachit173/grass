#include "Graph.h"

template <typename R, typename A>
Graph<R, A>::Graph(DistributedBuffer* buffer) {
    buffer_ = buffer;
    vertex_partitions_ = buffer_->GetPartitions();
}

template <typename R, typename A>
void Graph<R, A>::initialize() {
    vertex_partitions_ = buffer_->GetPartitions();
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
            graph::VertexPartition* src = interaction.src();
            graph::VertexPartition* dst = interaction.dst();
            graph::InteractionEdges* edges = interaction.edges();
            spdlog::trace("Processing interaction: src: {}, dst: {}", src->partition_id(), dst->partition_id());
            processInteraction(src, dst, edges);
            buffer_->MarkInteraction(interaction);
        }

        // Apply Phase
        vertex_partitions_ = buffer_->GetPartitions();
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
    vertex_partitions_ = buffer_->GetPartitions();
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
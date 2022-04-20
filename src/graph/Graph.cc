#include "Graph.h"

using graph::Double;
using graph::Int32;

template <typename R, typename A>
Graph<R, A>::Graph(std::string& graph_file, bool weighted_edges) {
    std::ifstream graph_file_stream(graph_file);
    if (!graph_file_stream.is_open()) {
        std::cerr << "Error opening file: " << graph_file << ". Exiting..." << std::endl;
        exit(1);
    }
    graph_file_stream >> num_vertices_;
    int src, dst;
    double weight = 1.0;
    while (graph_file_stream >> src >> dst) {
        graph::Edge* edge = new graph::Edge();
        edge->set_src(src);
        edge->set_dst(dst);
        if(weighted_edges) {
            graph_file_stream >> weight;
        }
        edge->set_weight(weight);
        // std::cout << "Edge: (" << edge.src() << ", " << edge.dst() << ") --> wt =" << edge.weight() << std::endl;
        edges_.push_back(Edge(edge));
    }
    num_edges_ = edges_.size();
    std::cout << "Number of edges: " << num_edges_ << std::endl;

    num_partitions_ = 2;
    vertex_partitions_ = std::vector<graph::VertexPartition>(num_partitions_);
    interaction_edges_ = std::vector<std::vector<graph::InteractionEdges>>(num_partitions_, std::vector<graph::InteractionEdges>(num_partitions_));
}

template <typename R, typename A>
void Graph<R, A>::initialize() {
    makePartitions();
    initializePartitions();
}

template <typename R, typename A>
void Graph<R, A>::startProcessing(const int &num_iters) {
    for (int iter = 0; iter < num_iters; iter++) {
        std::cout << "Iteration: " << iter << std::endl;
        // Gather Phase
        for (int i = 0; i < num_partitions_; i++) {
            for (int j = 0; j < num_partitions_; j++) {
                graph::VertexPartition* src =  &vertex_partitions_[i];
                graph::VertexPartition* dst = &vertex_partitions_[j];
                graph::InteractionEdges* edges = &interaction_edges_[i][j];
                processInteraction(src, dst, edges);
            }
        }

        // Apply Phase
        for(int i = 0; i < num_partitions_; i++) {
            applyPhase(vertex_partitions_[i]);
        }

        // Scatter phase
    }
}

template <typename R, typename A>
void Graph<R, A>::collectResults() {
    vertices_.clear();
    for (int i = 0; i < num_partitions_; i++) {
        for (int j = 0; j < vertex_partitions_[i].vertices().size(); j++) {
            graph::Vertex* vertex = vertex_partitions_[i].mutable_vertices(j);
            vertices_.push_back(Vertex<R, A>(vertex));
        }
    }
}

template <typename R, typename A>
std::vector< Vertex<R, A> >& Graph<R, A>::get_vertices() {
    return vertices_;
} 

template <typename R, typename A>
std::vector<Edge>& Graph<R, A>::get_edges() {
    return edges_;
}

template <typename R, typename A>
void Graph<R, A>::makePartitions() {
    int partition_size = (int)(ceil((double) num_vertices_ / (double)num_partitions_));
    std::cout << "Making Partitions of size: " << partition_size << std::endl;
    //1.  Divide number of vertices into partitions
    for(int i = 0; i < num_partitions_; i++) {
        int partition_start = i * partition_size, partition_end = std::min((int64_t)((i+1) * partition_size), num_vertices_);

        // Default initialization for Vertex
        for(int j = partition_start; j < partition_end; j++){ // j --> vertex id
            graph::Vertex* vertex = vertex_partitions_[i].add_vertices();
            vertex->set_id(j);
            vertex->mutable_degree()->set_out_degree(0);
            vertex->mutable_degree()->set_in_degree(0);
            // vertices_.push_back(Vertex(vertex));
        }
    }

    //2. Find edges between partitions
    // int64_t num_edges = edges.size();
    for(int64_t i = 0; i < num_edges_; i++) {
        int src = edges_[i].get_src(), dst = edges_[i].get_dst();
        int src_partition = src / partition_size, dst_partition = dst / partition_size;
        
        // Populate InteractionEdges
        graph::Edge *interEdge = interaction_edges_[src_partition][dst_partition].add_edges();
        *interEdge = edges_[i].get_edge();
        
        // Populate degrees        
        graph::Vertex *src_vertex = vertex_partitions_[src_partition].mutable_vertices(src % partition_size);
        graph::Vertex *dst_vertex = vertex_partitions_[dst_partition].mutable_vertices(dst % partition_size);
        src_vertex->mutable_degree()->set_out_degree(src_vertex->degree().out_degree() + 1);
        dst_vertex->mutable_degree()->set_in_degree(dst_vertex->degree().in_degree() + 1);
    }
}

template <typename R, typename A>
void Graph<R, A>::initializePartitions() {
    for (int i = 0; i < num_partitions_; i++) {
        for (int j = 0; j < vertex_partitions_[i].vertices().size(); j++) {
            graph::Vertex* vertex = vertex_partitions_[i].mutable_vertices(j);
            Vertex<R, A> v(vertex);
            init(v);
        }
    }
}

template <typename R, typename A>
void Graph<R, A>::processInteraction(graph::VertexPartition *src_partition, graph::VertexPartition *dst_partition, const graph::InteractionEdges *directed_edges) {
    int partition_size = (int)(ceil((double) num_vertices_ / (double)num_partitions_));
    int64_t num_interaction_edges = directed_edges->edges().size();
    
    std::vector<Vertex<R,A>> src_vertices, dst_vertices;
    for(int64_t i = 0; i < partition_size; i++) {
        if(i < src_partition->vertices().size()) {
            graph::Vertex* src = src_partition->mutable_vertices(i);
            src_vertices.emplace_back(Vertex<R, A>(src));
        }

        if(i < dst_partition->vertices().size()) {
            graph::Vertex* dst = dst_partition->mutable_vertices(i);
            dst_vertices.emplace_back(Vertex<R, A>(dst));
        }
    }
    Edge edge_obj;
    for(int64_t i = 0; i < num_interaction_edges; i++) {
        const graph::Edge *edge = &directed_edges->edges(i);
        
        // S[u] --> interaction.src[edge.src % partition_size]
        // S[v] --> interaction.dst[edge.dst % partition_size]
        // accumulator_add(S[v], S[u], edge)

        int src_vertex_id = edge->src(), dst_vertex_id = edge->dst();
        edge_obj.set_edge(edge);
        gather(src_vertices[src_vertex_id % partition_size], dst_vertices[dst_vertex_id % partition_size], edge_obj);
    }
}

template <typename R, typename A>
void Graph<R, A>::applyPhase(graph::VertexPartition& partition) {
    for (int i = 0; i < partition.vertices().size(); i++) {
        graph::Vertex* vertex = partition.mutable_vertices(i);
        Vertex<R, A> v(vertex);
        apply(v);
    }
}

template class Graph<double, double>;
// template class Graph<Int32, Int32>;
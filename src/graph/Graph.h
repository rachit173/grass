#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <grpcpp/grpcpp.h>
#include "protos/graph.grpc.pb.h"

#include "Vertex.h"
#include "Edge.h"
#include "src/distributed_buffer/distributed_buffer.h"

template <typename R, typename A>
class Graph {

public:
    Graph(DistributedBufferConfig config, std::string& graph_file, bool weighted_edges = false);
    void initialize();
    void startProcessing(const int &num_iters);
    void collectResults();
    std::vector<Vertex<R,A>>& get_vertices();
    std::vector<Edge>& get_edges();
    std::function<void (Vertex<R,A> &)> init_fn;
    std::function<void (Vertex<R,A>&, Vertex<R,A>&, const Edge&)> gather_fn;
    std::function<void (Vertex<R,A>&)> apply_fn;

private:
    int64_t num_vertices_;
    int64_t num_edges_;
    int num_partitions_;
    std::vector< Vertex<R,A> > vertices_;
    std::vector<Edge> edges_;
    std::vector<graph::VertexPartition*> vertex_partitions_;
    std::vector<std::vector<graph::InteractionEdges>> interaction_edges_;
    DistributedBuffer* buffer_;
    void makePartitions();
    void initializePartitions();
    void processInteraction(graph::VertexPartition *src_partition, graph::VertexPartition *dst_partition, const graph::InteractionEdges *directed_edges);
    void applyPhase(graph::VertexPartition& partition);
};

#endif // GRAPH_H
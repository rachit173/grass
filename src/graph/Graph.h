#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <spdlog/spdlog.h>
#include <grpcpp/grpcpp.h>
#include "protos/graph.grpc.pb.h"

#include "Vertex.h"
#include "Edge.h"
#include "src/distributed_buffer/distributed_buffer.h"

template <typename R, typename A>
class Graph {
public:
    typedef std::function<void (Vertex<R,A>&)> init_func_t;
    typedef std::function<void (Vertex<R,A>&, Vertex<R,A>&, const Edge&)> gather_func_t;
    typedef std::function<void (Vertex<R,A>&)> apply_func_t;

    Graph(DistributedBuffer* buffer);
    void initialize();
    void startProcessing(const int &num_iters);
    void collectResults();
    std::vector<Vertex<R,A>>& get_vertices();

protected:
    void set_init_func(init_func_t init_func);
    void set_gather_func(gather_func_t gather_func);
    void set_apply_func(apply_func_t apply_func);

private:
    int64_t num_vertices_;
    int64_t num_edges_;
    std::vector< Vertex<R,A> > vertices_;
    std::vector<Edge> edges_;
    std::vector<graph::VertexPartition*> vertex_partitions_;
    init_func_t init_func_;
    gather_func_t gather_func_;
    apply_func_t apply_func_;
    DistributedBuffer* buffer_;
    void processInteraction(graph::VertexPartition *src_partition, graph::VertexPartition *dst_partition, const graph::InteractionEdges *directed_edges);
    void applyPhase(graph::VertexPartition& partition);
};

#endif // GRAPH_H
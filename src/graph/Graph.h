#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <string>
#include <grpcpp/grpcpp.h>
#include "protos/graph.grpc.pb.h"

#include "Vertex.h"
#include "Edge.h"
#include "src/distributed_buffer/distributed_buffer.h"

template <typename R, typename A>
class Graph {

public:
    Graph(std::string& graph_file, bool weighted_edges = false);
    void initialize();
    void startProcessing(const int &num_iters);
    void collectResults();
    std::vector<Vertex<R,A>>& get_vertices();
    std::vector<Edge>& get_edges();

protected:
    virtual void init(Vertex<R,A> & vertex) = 0;
    virtual void gather(Vertex<R,A>& src, Vertex<R,A>& dst, const Edge& edge) = 0;
    virtual void apply(Vertex<R,A>& vertex) = 0;

private:
    int64_t num_vertices_;
    int64_t num_edges_;
    int num_partitions_;
    std::vector< Vertex<R,A> > vertices_;
    std::vector<Edge> edges_;
    std::vector<graph::VertexPartition> vertex_partitions_;
    std::vector<std::vector<graph::InteractionEdges>> interaction_edges_;
    void makePartitions();
    void initializePartitions();
    void processInteraction(graph::VertexPartition *src_partition, graph::VertexPartition *dst_partition, const graph::InteractionEdges *directed_edges);
    void applyPhase(graph::VertexPartition& partition);
};

#endif // GRAPH_H
#ifndef DEGREE_H
#define DEGREE_H

#include "protos/graph.grpc.pb.h"
#include "page_rank.h"

class Degree : public PageRank {
public:
    Degree(DistributedBufferConfig config, std::string& graph_file);
    void init(Vertex<double, double> & vertex) override;
    void gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) override;
    void apply(Vertex<double, double> & vertex) override;
};

#endif // DEGREE_H
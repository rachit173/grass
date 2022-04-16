#ifndef PAGE_RANK_H
#define PAGE_RANK_H

#include "protos/graph.grpc.pb.h"
#include "base_app.h"

using graph::Double;

class PageRank : public BaseApp<Double, Double> {
public:
    PageRank(std::string& graph_file);
    void init(Vertex<Double, Double> & vertex) override;
    void gather(Vertex<Double, Double> & src, Vertex<Double, Double>& dst, const Edge& edge) override;
    void apply(Vertex<Double, Double> & vertex) override;
};

#endif // PAGE_RANK_H
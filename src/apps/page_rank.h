#ifndef PAGE_RANK_H
#define PAGE_RANK_H

#include "protos/graph.grpc.pb.h"
#include "base_app.h"

using graph::Double;

class PageRank : public BaseApp<double, double> {
public:
    PageRank(std::string& graph_file);
    void init(Vertex<double, double> & vertex) override;
    void gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) override;
    void apply(Vertex<double, double> & vertex) override;
};

#endif // PAGE_RANK_H
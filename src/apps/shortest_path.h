#ifndef SHORTEST_PATH_H
#define SHORTEST_PATH_H

#include "protos/graph.grpc.pb.h"
#include "base_app.h"

using graph::Double;

class ShortestPath : public BaseApp<Double, Double> {
public:
    ShortestPath(std::string& graph_file, int64_t source_vtx_id);
    void init(Vertex<Double, Double> & vertex) override;
    void gather(Vertex<Double, Double> & src, Vertex<Double, Double>& dst, const Edge& edge) override;
    void apply(Vertex<Double, Double> & vertex) override;

private:
    int64_t src_vtx_id_;
};

#endif // SHORTEST_PATH_H
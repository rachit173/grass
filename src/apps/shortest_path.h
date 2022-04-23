#ifndef SHORTEST_PATH_H
#define SHORTEST_PATH_H

#include "protos/graph.grpc.pb.h"
#include "base_app.h"

class ShortestPath : public BaseApp<double, double> {
public:
    ShortestPath(std::string& graph_file, int64_t source_vtx_id);
    void init(Vertex<double, double> & vertex) override;
    void gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) override;
    void apply(Vertex<double, double> & vertex) override;

private:
    int64_t src_vtx_id_;
};

#endif // SHORTEST_PATH_H
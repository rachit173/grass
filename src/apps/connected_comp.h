#ifndef CONNECTED_COMP_H
#define CONNECTED_COMP_H

#include "protos/graph.grpc.pb.h"
#include "base_app.h"

using graph::Double;

class ConnectedComponents : public BaseApp<double, double> {
public:
    ConnectedComponents(std::string& graph_file);
    void init(Vertex<double, double> & vertex) override;
    void gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) override;
    void apply(Vertex<double, double> & vertex) override;
};

#endif // CONNECTED_COMP_H
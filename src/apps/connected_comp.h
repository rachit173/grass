#ifndef CONNECTED_COMP_H
#define CONNECTED_COMP_H

#include "protos/partition.grpc.pb.h"
#include "base_app.h"

using graph::Double;

class ConnectedComponents : public BaseApp<double, double> {
public:
    ConnectedComponents(DistributedBuffer* buffer, std::string input_file, bool weighted_edges = false);
    static void init(Vertex<double, double> & vertex);
    static void gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge);
    static void apply(Vertex<double, double> & vertex);
};

#endif // CONNECTED_COMP_H
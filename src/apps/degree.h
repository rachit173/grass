#ifndef DEGREE_H
#define DEGREE_H

#include "protos/partition.grpc.pb.h"
#include "base_app.h"

class Degree : public BaseApp<double, double> {
public:
    Degree(DistributedBuffer* buffer, std::string input_file, bool weighted_edges = false);
    static void init(Vertex<double, double> & vertex) ;
    static void gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) ;
    static void apply(Vertex<double, double> & vertex) ;
};

#endif // DEGREE_H
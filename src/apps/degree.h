#ifndef DEGREE_H
#define DEGREE_H

#include "protos/graph.grpc.pb.h"
#include "page_rank.h"

class Degree : public PageRank {
public:
    Degree(DistributedBufferConfig config, std::string& graph_file);
    static void init(Vertex<double, double> & vertex) ;
    static void gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) ;
    static void apply(Vertex<double, double> & vertex) ;
};

#endif // DEGREE_H
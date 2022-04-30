#ifndef DEGREE_H
#define DEGREE_H

#include "protos/graph.grpc.pb.h"
#include "page_rank.h"

class Degree : public PageRank {
public:
    Degree(DistributedBufferConfig config, std::string& graph_file);
    void set_fn_pointers();
};

void deg_init(Vertex<double, double> & vertex) ;
void deg_gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) ;
void deg_apply(Vertex<double, double> & vertex) ;

#endif // DEGREE_H
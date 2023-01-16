#ifndef PAGE_RANK_H
#define PAGE_RANK_H

#include "protos/partition.grpc.pb.h"
#include "base_app.h"

using graph::Double;

class PageRank : public BaseApp<double, double> {
public:
    PageRank(DistributedBuffer* buffer, std::string input_file, bool weighted_edges = false);
    static void init(Vertex<double, double> & vertex);
    static void gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge);
    static void apply(Vertex<double, double> & vertex);
};

#endif // PAGE_RANK_H
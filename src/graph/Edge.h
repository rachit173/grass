#ifndef EDGE_H
#define EDGE_H

#include <iostream>
#include <grpcpp/grpcpp.h>
#include "protos/graph.grpc.pb.h"

class Edge
{
public:
    Edge(const graph::Edge& edge){
        this->edge_ = edge;
    }

    int64_t get_src() const {
        return this->edge_.src();
    }

    int64_t get_dst() const {
        return this->edge_.dst();
    }

    double get_weight() const {
        return this->edge_.weight();
    }

    graph::Edge& get_edge() {
        return this->edge_;
    }
private:
    graph::Edge edge_;
};

#endif // EDGE_H
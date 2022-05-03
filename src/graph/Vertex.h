#ifndef VERTEX_H
#define VERTEX_H

#include <grpcpp/grpcpp.h>
#include "protos/graph.grpc.pb.h"

template <typename R, typename A> // For <result, accumulator>
class Vertex {
    public:
    Vertex() {
        this->vertex_ = nullptr;
    }
    
    Vertex(graph::Vertex *vertex){
        this->vertex_ = vertex;
    }
    
    int64_t get_id() {
        return this->vertex_->id();
    }

    int64_t get_indegree() {
        return this->vertex_->degree().in_degree();
    }

    int64_t get_outdegree() {
        return this->vertex_->degree().out_degree();
    }

    void set_indegree(int64_t in_degree) {
        this->vertex_->mutable_degree()->set_in_degree(in_degree);
    }

    void set_outdegree(int64_t out_degree) {
        this->vertex_->mutable_degree()->set_out_degree(out_degree);
    }

    void set_accumulator(A& acc) {
        this->vertex_->mutable_state()->set_accumulator(acc);
    }

    A get_accumulator() {
        return this->vertex_->state().accumulator();
    }

    void set_result(R& result) {
        this->vertex_->mutable_state()->set_result(result);
    }

    R get_result() {
        return this->vertex_->state().result();
    }

    private:
    graph::Vertex *vertex_;
};

#endif // VERTEX_H
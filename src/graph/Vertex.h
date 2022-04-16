#ifndef VERTEX_H
#define VERTEX_H

#include <grpcpp/grpcpp.h>
#include "protos/graph.grpc.pb.h"

template <typename R, typename A> // For <result, accumulator>
class Vertex {
    public:
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

    void set_accumulator(A& acc) {
        this->vertex_->mutable_state()->mutable_accumulator()->PackFrom(acc);
    }

    A get_accumulator() {
        A acc;
        this->vertex_->state().accumulator().UnpackTo(&acc);
        return acc;
    }

    void set_result(R& result) {
        this->vertex_->mutable_state()->mutable_result()->PackFrom(result);
    }

    R get_result() {
        R result;
        this->vertex_->state().result().UnpackTo(&result);
        return result;
    }

    private:
    graph::Vertex *vertex_;
};

#endif // VERTEX_H
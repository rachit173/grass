#include "page_rank.h"
#include <iostream>

using graph::Double;

PageRank::PageRank(DistributedBufferConfig config, std::string & graph_file) : BaseApp<double, double>(config, graph_file) {}

void PageRank::set_fn_pointers() {
    this->init_fn = &pr_init;
    this->gather_fn = &pr_gather;
    this->apply_fn = &pr_apply;
}

void pr_init(Vertex<double, double> & vertex) { 
    double init_val = 1.0;

    double init_accum_val = 0.0;

    vertex.set_result(init_val);
    vertex.set_accumulator(init_accum_val);
}

void pr_gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) {
    double prev_page_rank = src.get_result();
    int64_t out_degree = src.get_outdegree();

    double contribution = (prev_page_rank / (double) out_degree); 

    double acc = dst.get_accumulator();
    acc = acc + contribution;
    
    dst.set_accumulator(acc);
}

void pr_apply(Vertex<double, double> & vertex) {
    double acc = vertex.get_accumulator();

    double new_result = (1.0 - 0.85) + 0.85 * acc;
    vertex.set_result(new_result);

    double init_accum_val = 0.0;
    vertex.set_accumulator(init_accum_val);
}


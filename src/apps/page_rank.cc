#include "page_rank.h"

using graph::Double;

PageRank::PageRank(DistributedBuffer* buffer, std::string input_file, bool weighted_edges)
 : BaseApp<double, double>(buffer, input_file, weighted_edges) {
    this->Graph::set_init_func(&PageRank::init);
    this->Graph::set_gather_func(&PageRank::gather);
    this->Graph::set_apply_func(&PageRank::apply);
}

void PageRank::init(Vertex<double, double> & vertex) { 
    double init_val = 1.0;

    double init_accum_val = 0.0;

    vertex.set_result(init_val);
    vertex.set_accumulator(init_accum_val);
}

void PageRank::gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) {
    double prev_page_rank = src.get_result();
    int64_t out_degree = src.get_outdegree();

    double contribution = (prev_page_rank / (double) out_degree); 

    double acc = dst.get_accumulator();
    acc = acc + contribution;
    
    dst.set_accumulator(acc);
}

void PageRank::apply(Vertex<double, double> & vertex) {
    double acc = vertex.get_accumulator();

    double new_result = (1.0 - 0.85) + 0.85 * acc;
    vertex.set_result(new_result);

    double init_accum_val = 0.0;
    vertex.set_accumulator(init_accum_val);
}


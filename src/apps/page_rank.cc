#include "page_rank.h"

using graph::Double;

PageRank::PageRank(std::string & graph_file) : BaseApp<Double, Double>(graph_file) {}

void PageRank::init(Vertex<Double, Double> & vertex) { 
    Double init_val;
    init_val.set_value(1.0);

    Double init_accum_val;
    init_accum_val.set_value(0.0);

    vertex.set_result(init_val);
    vertex.set_accumulator(init_accum_val);
}

void PageRank::gather(Vertex<Double, Double> & src, Vertex<Double, Double>& dst, const Edge& edge) {
    Double prev_page_rank = src.get_result();
    int64_t out_degree = src.get_outdegree();

    double contribution = (prev_page_rank.value() / (double) out_degree); 

    Double acc = dst.get_accumulator();
    acc.set_value(acc.value() + contribution);
    
    dst.set_accumulator(acc);
}

void PageRank::apply(Vertex<Double, Double> & vertex) {
    Double acc = vertex.get_accumulator();

    Double new_result;
    new_result.set_value((1.0 - 0.85) + 0.85 * acc.value());
    vertex.set_result(new_result);

    Double init_accum_val;
    init_accum_val.set_value(0.0);
    vertex.set_accumulator(init_accum_val);
}


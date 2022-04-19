#include "shortest_path.h"

ShortestPath::ShortestPath(std::string& filename, int64_t src_vtx_id) : BaseApp<Double, Double> (filename), src_vtx_id_(src_vtx_id) {}

void ShortestPath::init(Vertex<Double, Double> & vertex) {
    Double init_dist;
    if(vertex.get_id() == src_vtx_id_) {
        init_dist.set_value(0.0);
    } else {
        init_dist.set_value(1e9);
    }
    vertex.set_result(init_dist);
    vertex.set_accumulator(init_dist);
}

void ShortestPath::gather(Vertex<Double, Double> & src, Vertex<Double, Double>& dst, const Edge& edge) {
    Double src_dist = src.get_accumulator();
    Double dst_dist = dst.get_accumulator();
    Double new_dist;
    new_dist.set_value(src_dist.value() + edge.get_weight());
    if(new_dist.value() < dst_dist.value()) {
        dst.set_accumulator(new_dist);
    }
}

void ShortestPath::apply(Vertex<Double, Double> & vertex) {
    Double new_dist = vertex.get_accumulator();
    if(new_dist.value() < vertex.get_result().value()) {
        vertex.set_result(new_dist);
    }
}
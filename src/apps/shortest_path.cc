#include "shortest_path.h"

int64_t ShortestPath::src_vtx_id_ = 0;

ShortestPath::ShortestPath(DistributedBufferConfig config, std::string& filename, int64_t src_vtx_id) : BaseApp<double, double> (config, filename) {
    this->Graph::set_init_func(&ShortestPath::init);
    this->Graph::set_gather_func(&ShortestPath::gather);
    this->Graph::set_apply_func(&ShortestPath::apply);
    this->src_vtx_id_ = src_vtx_id;
}

void ShortestPath::init(Vertex<double, double> & vertex) {
    double init_dist = 1e9;
    if(vertex.get_id() == src_vtx_id_) {
        init_dist = 0.0;
    }
    vertex.set_result(init_dist);
    vertex.set_accumulator(init_dist);
}

void ShortestPath::gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) {
    double src_dist = src.get_accumulator();
    double dst_dist = dst.get_accumulator();
    double new_dist;
    new_dist = src_dist + edge.get_weight();
    if(new_dist < dst_dist) {
        dst.set_accumulator(new_dist);
    }
}

void ShortestPath::apply(Vertex<double, double> & vertex) {
    double new_dist = vertex.get_accumulator();
    if(new_dist < vertex.get_result()) {
        vertex.set_result(new_dist);
    }
}
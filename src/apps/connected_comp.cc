#include "connected_comp.h"

ConnectedComponents::ConnectedComponents(std::string & graph_file) : BaseApp<double, double>(graph_file) {}

void ConnectedComponents::init(Vertex<double, double> & vertex) { 
    double init_val = vertex.get_id();
    double init_accum_val = vertex.get_id();

    vertex.set_result(init_val);
    vertex.set_accumulator(init_accum_val);
}

void ConnectedComponents::gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) {
    double src_comp = src.get_result(), dst_comp = dst.get_result();
    double acc_comp = std::min(src_comp, dst_comp);
    double new_src_comp = std::min(src.get_accumulator(), acc_comp);
    double new_dst_comp = std::min(dst.get_accumulator(), acc_comp);
    
    src.set_accumulator(new_src_comp);
    dst.set_accumulator(new_dst_comp);
}

void ConnectedComponents::apply(Vertex<double, double> & vertex) {
    double acc = vertex.get_accumulator();
    double new_result = std::min(vertex.get_result(), acc);
    vertex.set_result(new_result);
}

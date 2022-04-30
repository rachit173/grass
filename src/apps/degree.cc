#include "degree.h"

Degree::Degree(DistributedBufferConfig config, std::string& graph_file) : PageRank(config, graph_file) {}
void Degree::set_fn_pointers() {
    this->init_fn = &deg_init;
    this->gather_fn = &deg_gather;
    this->apply_fn = &deg_apply;
}

void deg_init(Vertex<double, double> & vertex) {
    vertex.set_indegree(0);
    vertex.set_outdegree(0);
}
void deg_gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) {
    src.set_outdegree(src.get_outdegree() + 1);
    dst.set_indegree(dst.get_indegree() + 1);
}
void deg_apply(Vertex<double, double> & vertex) {
    double degree = (vertex.get_indegree() + vertex.get_outdegree());
    vertex.set_result(degree);
}
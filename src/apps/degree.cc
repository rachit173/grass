#include "degree.h"

Degree::Degree(DistributedBufferConfig config, std::string& graph_file) : PageRank(config, graph_file) {}
void Degree::init(Vertex<double, double> & vertex) {
    vertex.set_indegree(0);
    vertex.set_outdegree(0);
}
void Degree::gather(Vertex<double, double> & src, Vertex<double, double>& dst, const Edge& edge) {
    src.set_outdegree(src.get_outdegree() + 1);
    dst.set_indegree(dst.get_indegree() + 1);
}
void Degree::apply(Vertex<double, double> & vertex) {
    double degree = (vertex.get_indegree() + vertex.get_outdegree());
    vertex.set_result(degree);
}

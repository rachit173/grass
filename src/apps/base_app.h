#ifndef BASE_APP_H
#define BASE_APP_H

#include "protos/graph.grpc.pb.h"
#include "src/graph/Graph.h"
#include "src/distributed_buffer/distributed_buffer.h"


template <typename R, typename A>
class BaseApp : public Graph<R, A> {
protected:
    BaseApp(DistributedBufferConfig config, std::string& graph_file, bool weighted_edges = false): Graph<R, A>(config, graph_file, weighted_edges) {}


///////////////////////// Methods to be implemented ///////////////////////////////
//  void init(Vertex<R, A> & vertex);
//  void gather(Vertex<R, A> & src, Vertex<R, A>& dst, const Edge& edge);
//  void apply(Vertex<R, A> & vertex);
///////////////////////////////////////////////////////////////////////////////////



//////////////////////////////// APIs available ///////////////////////////////////
//  void initialize();
//  void startProcessing(const int &num_iters);
//  void collectResults();
//  std::vector<Vertex<R,A>>& get_vertices();
//  std::vector<Edge>& get_edges();
///////////////////////////////////////////////////////////////////////////////////

};

#endif // BASE_APP
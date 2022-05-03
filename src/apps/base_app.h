#ifndef BASE_APP_H
#define BASE_APP_H

#include "protos/graph.grpc.pb.h"
#include "src/graph/Graph.h"
#include "src/distributed_buffer/distributed_buffer.h"


template <typename R, typename A>
class BaseApp : public Graph<R, A> {
protected:
    BaseApp(DistributedBufferConfig config, std::string& graph_file, bool weighted_edges = false): Graph<R, A>(config, graph_file, weighted_edges) {}


///////////////////////// Static Methods to be implemented ///////////////////////////////
//  static void init(Vertex<R, A> & vertex);
//  static void gather(Vertex<R, A> & src, Vertex<R, A>& dst, const Edge& edge);
//  static void apply(Vertex<R, A> & vertex);
//  static variables (if any)
///////////////////////////////////////////////////////////////////////////////////


//////////////////////////////// APIs available ///////////////////////////////////
//  void set_init_func(init_func);
//  void set_gather_func(gather_func);
//  void set_apply_func(apply_func);
//  void initialize();
//  void startProcessing(const int &num_iters);
//  void collectResults();
//  std::vector<Vertex<R,A>>& get_vertices();
//  std::vector<Edge>& get_edges();
///////////////////////////////////////////////////////////////////////////////////

};

#endif // BASE_APP
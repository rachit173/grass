#ifndef BASE_APP_H
#define BASE_APP_H

#include "protos/graph.grpc.pb.h"
#include "src/graph/Graph.h"


template <typename R, typename A>
class BaseApp : public Graph<R, A> {
protected:
    BaseApp(std::string& graph_file): Graph<R, A>(graph_file) {}


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
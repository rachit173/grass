#include <stdio.h>
#include <string>
#include <fstream>
#include "protos/graph.grpc.pb.h"
#include "src/apps/page_rank.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <graph_data>" << std::endl;
    return 1;
    }
    std::string filename = argv[1];
    PageRank* app = new PageRank(filename);
    app->initialize();
    app->startProcessing(2);
    app->collectResults();
    
    auto vertices = app->get_vertices();
    auto edges = app->get_edges();

    std::ofstream outfile;
    outfile.open("/mnt/Work/grass/resources/graphs/calculated_pagerank_using_api.txt");

    for (auto &vertex: vertices) {
        Double result = vertex.get_result();
        outfile << vertex.get_id() << " " << result.value() << std::endl;
    }
}
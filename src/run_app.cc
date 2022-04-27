#include <stdio.h>
#include <string>
#include <fstream>
#include "protos/graph.grpc.pb.h"
#include "src/apps/page_rank.h"
#include "src/apps/connected_comp.h"
#include "src/apps/shortest_path.h"
#include "src/apps/degree.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <base dir> <output dir> <iterations>" << std::endl;
        return 1;
    }

    std::string base_dir = argv[1];
    int iterations = atoi(argv[3]);
    
    std::string filename = "web-BerkStan.txt";
    std::string filepath = base_dir + "/graphs/" + filename;

    DistributedBufferConfig config;
    config.self_rank = 0;
    config.capacity = 4;
    config.num_partitions = 4;
    config.num_workers = 1;
    config.server_addresses = {"localhost:50051"};

    BaseApp<double, double>* app;

    // app = new PageRank(config, filepath);
    // app = new Degree(config, filepath);
    // app = new ShortestPath(config, filepath, 1);
    app = new ConnectedComponents(config, filepath);

    auto start = std::chrono::high_resolution_clock::now();

    app->initialize();
    app->startProcessing(iterations);
    app->collectResults();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
    
    auto vertices = app->get_vertices();
    auto edges = app->get_edges();

    std::ofstream outfile;
    std::string outdir = argv[2];
    outfile.open(outdir + "/actual_results/" + filename);

    for (auto &vertex: vertices) {
        double result = vertex.get_result();
        outfile << vertex.get_id() << " " << result << std::endl;
    }
}
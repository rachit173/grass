#include <stdio.h>
#include <string>
#include <fstream>
#include "protos/graph.grpc.pb.h"
#include "src/apps/page_rank.h"
#include "src/apps/connected_comp.h"
#include "src/apps/shortest_path.h"
#include "src/apps/degree.h"
#include "src/utils/config_utils.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " rank <config filepath>" << std::endl;
        return 1;
    }
    unordered_map<string, string> config;
    DistributedBufferConfig buffer_config;
    string config_file = argv[2];
    if(parse_config_file(config_file, config) != 0) {
        std::cout << "Failed to parse config file" << std::endl;
        return -1;
    }

    // Read app config
    std::string base_dir = config["app.base_dir"];
    std::string app_name = config["app.name"];
    std::string outdir = base_dir + "/" + app_name;
    int iterations = std::stoi(config["app.iterations"]);
    std::string filename = config["app.graph_file"];
    std::string filepath = base_dir + "/graphs/" + filename;

    // Read buffer config
    buffer_config.self_rank = std::stoi(argv[1]);
    buffer_config.capacity = stoi(config["buffer.capacity"]);
    buffer_config.num_partitions = stoi(config["buffer.num_partitions"]);
    buffer_config.num_workers = stoi(config["buffer.num_workers"]);
    buffer_config.server_addresses = split_addresses(config["buffer.server_addresses"]);

    Degree* app;

    // app = new PageRank(buffer_config, filepath);
    app = new Degree(buffer_config, filepath);
    app->set_fn_pointers();
    // app = new ShortestPath(buffer_config, filepath, 1);
    // app = new ConnectedComponents(buffer_config, filepath);

    auto start = std::chrono::high_resolution_clock::now();

    app->initialize();
    app->startProcessing(1);
    app->collectResults();

    // PageRank* app2 = (PageRank*)app;
    // app2->set_fn_pointers();
    // app2->initialize();
    // app2->startProcessing(iterations);
    // app2->collectResults();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
    
    auto vertices = app->get_vertices();
    auto edges = app->get_edges();

    std::ofstream outfile;
    filename = filename + "_" + std::to_string(buffer_config.self_rank);
    outfile.open(outdir + "/actual_results/" + filename);

    for (auto &vertex: vertices) {
        double result = vertex.get_result();
        outfile << vertex.get_id() << " " << result << std::endl;
    }
}
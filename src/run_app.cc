#include <stdio.h>
#include <string>
#include <fstream>
#include <spdlog/spdlog.h>
#include "protos/graph.grpc.pb.h"
#include "src/apps/page_rank.h"
#include "src/apps/connected_comp.h"
#include "src/apps/shortest_path.h"
#include "src/apps/degree.h"
#include "src/utils/config_utils.h"
#include "src/utils/logger.h"

using namespace std;

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");

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
    std::string outdir = base_dir + "/../" + app_name;
    int iterations = std::stoi(config["app.iterations"]);
    std::string filename = config["app.graph_file"];
    std::string filepath = base_dir + "/" + filename;
    std::string log_level = config["app.log_level"];
    std::string log_file = app_name + "_" + filename;

    // Read buffer config
    buffer_config.self_rank = std::stoi(argv[1]);
    buffer_config.capacity = stoi(config["buffer.capacity"]);
    buffer_config.num_partitions = stoi(config["buffer.num_partitions"]);
    buffer_config.num_workers = stoi(config["buffer.num_workers"]);
    buffer_config.server_addresses = split_addresses(config["buffer.server_addresses"]);

    GrassLogger grass_logger = GrassLogger(log_file);
    spdlog::set_default_logger(grass_logger.main_logger_);
    
    // Set log level for console
    spdlog::level::level_enum log_level_enum = spdlog::level::from_str(log_level);
    grass_logger.setConsoleLogLevel(log_level_enum);

    auto start = std::chrono::high_resolution_clock::now();
    DistributedBuffer* buffer = new DistributedBuffer(buffer_config, filepath);
    Degree* degree = new Degree(buffer);
    PageRank* pagerank = new PageRank(buffer);

    // app = new ShortestPath(buffer, 1);
    // app = new ConnectedComponents(buffer);


    degree->initialize();
    degree->startProcessing(1);

    pagerank->initialize();
    pagerank->startProcessing(iterations);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    spdlog::info("Execution time: {} ms", duration.count());
    
    auto collect_result_start = std::chrono::high_resolution_clock::now();
    pagerank->collectResults();
    auto vertices = pagerank->get_vertices();
    auto collect_result_end = std::chrono::high_resolution_clock::now();
    auto collect_result_duration = std::chrono::duration_cast<std::chrono::milliseconds>(collect_result_end - collect_result_start);
    spdlog::info("Collect result time: {} ms", collect_result_duration.count());

    std::ofstream outfile;
    filename = filename + "_" + std::to_string(buffer_config.self_rank);
    outfile.open(outdir + "/actual_results/" + filename);

    auto write_start = std::chrono::high_resolution_clock::now();
    for (auto &vertex: vertices) {
        double result = vertex.get_result();
        outfile << vertex.get_id() << " " << result << std::endl;
    }
    auto write_end = std::chrono::high_resolution_clock::now();
    auto write_duration = std::chrono::duration_cast<std::chrono::milliseconds>(write_end - write_start);
    spdlog::info("Write time: {} ms", write_duration.count());
}
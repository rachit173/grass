#include <stdio.h>
#include <string>
#include <fstream>
#include <spdlog/spdlog.h>
#include "protos/partition.grpc.pb.h"
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
    std::string input_dir = config["app.input_dir"];
    std::string output_dir = config["app.output_dir"];
    std::string app_name = config["app.name"];
    std::string outdir = output_dir + "/" + app_name;
    int iterations = std::stoi(config["app.iterations"]);
    std::string filename = config["app.graph_file"];
    std::string filepath = input_dir + "/" + filename;

    // Read buffer config
    buffer_config.self_rank = std::stoi(argv[1]);
    buffer_config.capacity = stoi(config["buffer.capacity"]);
    buffer_config.num_partitions = stoi(config["buffer.num_partitions"]);
    buffer_config.num_workers = stoi(config["buffer.num_workers"]);
    buffer_config.server_addresses = split_addresses(config["buffer.server_addresses"]);

    std::string log_level = config["app.log_level"];
    std::string log_file = app_name + "_" + filename + to_string(buffer_config.self_rank);
    GrassLogger grass_logger = GrassLogger(log_file);
    spdlog::set_default_logger(grass_logger.main_logger_);
    
    // Set log level for console
    spdlog::level::level_enum log_level_enum = spdlog::level::from_str(log_level);
    grass_logger.setConsoleLogLevel(log_level_enum);
    
    PartitionType partition_type = PartitionType::kVertexPartition;


    auto p1 = std::chrono::high_resolution_clock::now();
    DistributedBuffer* buffer = new DistributedBuffer(buffer_config, partition_type);
    if (app_name == "pagerank") {
        Degree* degree = new Degree(buffer, filepath);
        PageRank* pagerank = new PageRank(buffer, filepath);
        auto p2 = std::chrono::high_resolution_clock::now();
        degree->initialize();
        degree->startProcessing(1);

        pagerank->initialize();
        pagerank->startProcessing(iterations);
        auto p3 = std::chrono::high_resolution_clock::now();
        auto load_data_duration = std::chrono::duration_cast<std::chrono::milliseconds>(p2 - p1);
        auto process_duration = std::chrono::duration_cast<std::chrono::milliseconds>(p3 - p2);
        spdlog::info("Load Data time: {} ms", load_data_duration.count());
        spdlog::info("Execution time: {} ms", process_duration.count());
        buffer->WriteMetrics();
        degree->WriteMetrics("degree");
        pagerank->WriteMetrics("pagerank");
    } else if (app_name == "connectedcomps") {
        ConnectedComponents* comps = new ConnectedComponents(buffer, filepath);
        auto p2 = std::chrono::high_resolution_clock::now();
        comps->initialize();
        comps->startProcessing(iterations);
        auto p3 = std::chrono::high_resolution_clock::now();
        auto load_data_duration = std::chrono::duration_cast<std::chrono::milliseconds>(p2 - p1);
        auto process_duration = std::chrono::duration_cast<std::chrono::milliseconds>(p3 - p2);
        spdlog::info("Load Data time: {} ms", load_data_duration.count());
        spdlog::info("Execution time: {} ms", process_duration.count());
        buffer->WriteMetrics();
        comps->WriteMetrics("comps");
    } else if (app_name == "shortestpath") {
        ShortestPath* sssp = new ShortestPath(buffer, filepath);
        auto p2 = std::chrono::high_resolution_clock::now();
        sssp->initialize();
        sssp->startProcessing(iterations);
        auto p3 = std::chrono::high_resolution_clock::now();
        auto load_data_duration = std::chrono::duration_cast<std::chrono::milliseconds>(p2 - p1);
        auto process_duration = std::chrono::duration_cast<std::chrono::milliseconds>(p3 - p2);
        spdlog::info("Load Data time: {} ms", load_data_duration.count());
        spdlog::info("Execution time: {} ms", process_duration.count());
        buffer->WriteMetrics();
        sssp->WriteMetrics("sssp");
    } else {
        std::cout << "Unknown app name" << std::endl;
        return -1;
    }
}
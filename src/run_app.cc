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
    auto p2 = std::chrono::high_resolution_clock::now();
    Degree* degree = new Degree(buffer, filepath);
    PageRank* pagerank = new PageRank(buffer, filepath);

    // app = new ShortestPath(buffer, 1);
    // app = new ConnectedComponents(buffer);


    degree->initialize();
    degree->startProcessing(1);

    pagerank->initialize();
    pagerank->startProcessing(iterations);

    auto p3 = std::chrono::high_resolution_clock::now();
    auto load_data_duration = std::chrono::duration_cast<std::chrono::milliseconds>(p2 - p1);
    auto process_duration = std::chrono::duration_cast<std::chrono::milliseconds>(p3 - p2);
    spdlog::info("Load Data time: {} ms", load_data_duration.count());
    spdlog::info("Execution time: {} ms", process_duration.count());
    
    auto collect_result_start = std::chrono::high_resolution_clock::now();
    pagerank->collectResults();
    auto vertices = pagerank->get_vertices();
    auto collect_result_end = std::chrono::high_resolution_clock::now();
    auto collect_result_duration = std::chrono::duration_cast<std::chrono::milliseconds>(collect_result_end - collect_result_start);
    spdlog::info("Collect result time: {} ms", collect_result_duration.count());

    std::ofstream outfile;
    filename = filename + "_" + std::to_string(buffer_config.self_rank);
    std::string outfilepath = outdir + "/actual_results/" + filename;
    outfile.open(outfilepath);

    if (!outfile.is_open()) {
        spdlog::error("Failed to open file {}", outfilepath);
        return -1;
    }

    auto write_start = std::chrono::high_resolution_clock::now();
    for (auto &vertex: vertices) {
        double result = vertex.get_result();
        outfile << vertex.get_id() << " " << result << std::endl;
    }
    auto write_end = std::chrono::high_resolution_clock::now();
    auto write_duration = std::chrono::duration_cast<std::chrono::milliseconds>(write_end - write_start);
    spdlog::info("Write time: {} ms", write_duration.count());
}
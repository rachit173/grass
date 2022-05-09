#include <stdio.h>
#include <string>
#include <fstream>
#include <spdlog/spdlog.h>

#include "src/matmul/apps/attention_matmul.h"
#include "src/utils/config_utils.h"
#include "src/utils/logger.h"

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
    std::string log_file = app_name + "_" + filename + "_" + std::to_string(buffer_config.self_rank);
    GrassLogger grass_logger = GrassLogger(log_file);
    spdlog::set_default_logger(grass_logger.main_logger_);
    
    // Set log level for console
    spdlog::level::level_enum log_level_enum = spdlog::level::from_str(log_level);
    grass_logger.setConsoleLogLevel(log_level_enum);

    PartitionType partition_type = PartitionType::kMatrixPartition;

    auto start = std::chrono::high_resolution_clock::now();
    DistributedBuffer* buffer = new DistributedBuffer(buffer_config, partition_type);
    AttentionMatrixMultiply *attention_mm = new AttentionMatrixMultiply(buffer, filepath);

    attention_mm->initialize();
    attention_mm->startProcessing(iterations);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    spdlog::info("Execution time: {} ms", duration.count());

    std::ofstream outfile;
    filename = filename + "_" + std::to_string(buffer_config.self_rank);
    std::string outfilepath = outdir + "/actual_results/" + filename;
    outfile.open(outfilepath);

    if (!outfile.is_open()) {
        spdlog::error("Failed to open file {}", outfilepath);
        return -1;
    }

    
    // Collect results from other ranks
    if (buffer_config.self_rank == 0) {
        auto collect_result_start = std::chrono::high_resolution_clock::now();
        attention_mm->collectResults();
        auto collect_result_end = std::chrono::high_resolution_clock::now();
        auto collect_result_duration = std::chrono::duration_cast<std::chrono::milliseconds>(collect_result_end - collect_result_start);
        spdlog::info("Collect result time: {} ms", collect_result_duration.count());
        
        Matrix_t result_matrix = attention_mm->GetResultMatrix();
        outfile << result_matrix.to_string(false);
    } else {
        spdlog::info("Waiting for results from rank 0");
        sleep(buffer_config.num_workers*10);
    }

    outfile.close();
}
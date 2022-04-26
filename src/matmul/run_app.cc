// Reference: https://icl.bitbucket.io/blaspp/group__gemm.html

#include <iostream>
#include <blas.hh>
#include <spdlog/spdlog.h>

#include "src/matmul/apps/attention_matmul.h"

int main( int argc, char** argv){
    spdlog::set_level(spdlog::level::info);

    int num_iters, num_partitions;
    if (argc != 3) {
        spdlog::warn("Usage: ./run_app <num_iters> <num_partitions>. Using defaults..");
        num_iters = 1;
        num_partitions = 2;
    } else {
        num_iters = atoi(argv[1]);
        num_partitions = atoi(argv[2]);
    }

    std::string matA_input_file = "/mnt/Work/grass/resources/matmul/matA.txt";

    spdlog::info("Running Attention Matrix Multiplication with {} iters and {} partitions", num_iters, num_partitions);
    AttentionMatrixMultiply *attention_mm = new AttentionMatrixMultiply(num_partitions, matA_input_file);

    attention_mm->initialize();
    attention_mm->startProcessing(num_iters);
    attention_mm->collectResults();

    attention_mm->show_input_matrix();
    attention_mm->show_result_matrix();
    return 0;
}

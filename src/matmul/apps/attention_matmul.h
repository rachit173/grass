#ifndef ATTENTION_MATMUL_H
#define ATTENTION_MATMUL_H

// Reference: https://icl.bitbucket.io/blaspp/group__gemm.html

#include <spdlog/spdlog.h>
#include <blas.hh>

#include "base_matmul.h"

class AttentionMatrixMultiply: public BaseMatMulApp<double> {
public:
    AttentionMatrixMultiply(DistributedBuffer* buffer, std::string &input_file);
    static void matmul_using_blas(Matrix<double> &matrix_A, Matrix<double> &matrix_B, int64_t result_idx);
    void show_input_matrix();
    void show_result_matrix();
};

#endif // ATTENTION_MATMUL_H

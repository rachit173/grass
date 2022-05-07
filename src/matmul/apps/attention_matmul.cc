#include "attention_matmul.h"

AttentionMatrixMultiply::AttentionMatrixMultiply(DistributedBuffer* buffer, std::string &input_file)
: BaseMatMulApp<double>(buffer, input_file) {
    this->MatMul::set_matmul_func(&AttentionMatrixMultiply::matmul_using_blas);
}

void AttentionMatrixMultiply::matmul_using_blas(Matrix<double> &matrix_A, Matrix<double> &matrix_B, int64_t result_idx) {
    double alpha = 1.0, beta = 0.0;
    int64_t m = matrix_A.get_num_rows();
    int64_t k = matrix_A.get_num_cols();
    int64_t n = matrix_B.get_num_rows();

    if (m == 0 || n == 0) {
        spdlog::warn("Skipping Multiplication of empty matrix");
        return;
    }
    
    int64_t lda = k;
    int64_t ldb = k;
    int64_t ldc = n;

    const double *A = matrix_A.get_data();
    const double *B = matrix_B.get_data();
    double *C = matrix_A.get_mutable_results(result_idx);

    try {        
        blas::gemm(blas::Layout::RowMajor, blas::Op::NoTrans, blas::Op::Trans,
                    m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
    } catch (const std::exception& e) {
        spdlog::error("Using blas to multiply matrix A with matrix B failed. Error: {}", e.what());
        spdlog::error("m: {}, k: {}, n: {}", m, k, n);
        spdlog::error("lda: {}, ldb: {}, ldc: {}", lda, ldb, ldc);
        exit(1);
    }
}

void AttentionMatrixMultiply::show_input_matrix() {
    Matrix<double> input_matrix = this->GetInputMatrix();
    std::cout << "Input matrix: \n" << input_matrix.to_string() << std::endl;
}

void AttentionMatrixMultiply::show_result_matrix() {
    Matrix<double> result_matrix = this->GetResultMatrix();
    std::cout << "Result matrix: \n" << result_matrix.to_string() << std::endl;
}
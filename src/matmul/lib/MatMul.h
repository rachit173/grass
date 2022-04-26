#ifndef MATMUL_PROTO_H
#define MATMUL_PROTO_H

#include <spdlog/spdlog.h>
#include <functional>
#include <fstream>

#include <grpcpp/grpcpp.h>
#include "protos/matmul.grpc.pb.h"

#include "Matrix.h"

template< typename T>
class MatMul {
public:
    typedef std::function<void (Matrix<T> &, Matrix<T> &, int64_t)> matmul_func_t;
    
    MatMul<T>(int num_partitions_A, std::string &matA_input_file);
    void initialize();
    void startProcessing(int num_iters);
    void collectResults();

    Matrix<T> &GetResultMatrix();
    Matrix<T> &GetInputMatrix();

protected:
    void set_matmul_func(matmul_func_t matmul_func);

private:
    void read_data(std::string &filename);
    void makePartitions();

    int num_partitions_A_;
    int num_rows_, num_cols_;
    std::vector<matmul::MatrixPartition*> matrix_partitions_;
    std::vector<T> data_;
    Matrix<T> *input_matrix_;
    Matrix<T> *result_matrix_;
    matmul_func_t matmul_func_;
};

#endif // MATMUL_PROTO_H

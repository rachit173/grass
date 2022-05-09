#ifndef MATMUL_PROTO_H
#define MATMUL_PROTO_H

#include <spdlog/spdlog.h>
#include <functional>
#include <fstream>

#include <grpcpp/grpcpp.h>
#include "protos/partition.grpc.pb.h"

#include "Matrix.h"
#include "src/distributed_buffer/distributed_buffer.h"

template< typename T>
class MatMul {
public:
    typedef std::function<void (Matrix<T> &, Matrix<T> &, int64_t)> matmul_func_t;
    
    MatMul<T>(DistributedBuffer* buffer, std::string &input_file);
    MatMul<T>(DistributedBuffer* buffer, std::string &input_file1, std::string &input_file2);
    void initialize();
    void startProcessing(int num_iters);
    void collectResults();

    Matrix<T> &GetResultMatrix();
    Matrix<T> &GetInputMatrix();

protected:
    void set_matmul_func(matmul_func_t matmul_func);

private:
    void LoadInteractions();
    void InitPartition(partition::Partition *partition, int partition_start, int partition_end);
    void processInteraction(matmul::MatrixPartition *src_partition, matmul::MatrixPartition *dst_partition);

    int num_partitions_A_;
    int num_rows_, num_cols_;
    std::vector<matmul::MatrixPartition*> matrix_partitions_;
    std::vector<T> data_;
    Matrix<T> *input_matrix_;
    Matrix<T> *result_matrix_;
    matmul_func_t matmul_func_;
    DistributedBuffer* buffer_;
    std::string input_file_;

};

#endif // MATMUL_PROTO_H

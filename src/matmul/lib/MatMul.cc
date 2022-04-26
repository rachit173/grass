#include "MatMul.h"

template< typename T>
MatMul<T>::MatMul(int num_partitions_A, std::string &matA_input_file):num_partitions_A_(num_partitions_A) {
    for (int i = 0; i < num_partitions_A; i++) {
        matrix_partitions_.push_back(new matmul::MatrixPartition());
    }
    read_data(matA_input_file);
}

template< typename T>
void MatMul<T>::read_data(std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::invalid_argument("MatMul::read_from_file: file not found");
    file >> num_rows_ >> num_cols_;
    data_.resize(num_rows_*num_cols_);

    for(int64_t i = 0; i< num_rows_*num_cols_; i++) {
        file >> data_[i];
    }
}

template< typename T>
void MatMul<T>::makePartitions() {
    for(int i = 0; i < num_partitions_A_; i++) {
        int64_t start_row = i * num_rows_ / num_partitions_A_;
        int64_t end_row = ((i+1) * num_rows_ / num_partitions_A_);
        int64_t start_col = 0;
        int64_t end_col = num_cols_;
        
        matmul::Matrix *matrix = matrix_partitions_[i]->mutable_matrix();
        matrix->set_id(i);
        matrix->set_num_rows(end_row - start_row);
        matrix->set_num_cols(end_col - start_col);
        
        // Data is stored sequentially in vector data. Note: End row is exclusive
        int64_t start_index = start_row * num_cols_ + start_col;
        int64_t end_index = (end_row - 1) * num_cols_ + end_col;

        for(int64_t j = start_index; j < end_index; j++) {
            matrix->mutable_state()->add_values(data_[j]);
        }
        
        int64_t row_range = (end_row - start_row) * (end_row - start_row);
        for(int k = 0; k < num_partitions_A_; k++) {
            matmul::MatrixState_Result *results = matrix->mutable_state()->add_results();
            for(int64_t j = 0; j < row_range; j++) {
                results->add_values(0);
            }
        }
        spdlog::debug("Matrix Partition {} has a Matrix of Size: {} X {}", i, matrix->num_rows(), matrix->num_cols());
    }
}

template< typename T>
void MatMul<T>::initialize() {
    makePartitions();
}

template< typename T>
void MatMul<T>::startProcessing(int num_iters) {
    for(int iter = 0; iter < num_iters; iter++) {
        for(int i = 0; i < num_partitions_A_; i++) {
            for(int j = 0; j < num_partitions_A_; j++) {
                matmul::Matrix *matrix_A = matrix_partitions_[i]->mutable_matrix();
                matmul::Matrix *matrix_B = matrix_partitions_[j]->mutable_matrix();

                Matrix<T> matrix_A_proto(matrix_A);
                Matrix<T> matrix_B_proto(matrix_B);
                matmul_func_(matrix_A_proto, matrix_B_proto, j);
            }
        }
    }
}

template< typename T>
void MatMul<T>::collectResults() {
    matmul::Matrix *result_mat = new matmul::Matrix();
    result_mat->set_id(0);
    result_mat->set_num_rows(num_rows_);
    result_mat->set_num_cols(num_rows_);

    for(int64_t i = 0; i < num_rows_ * num_rows_; i++) {
        result_mat->mutable_state()->add_values(0);
    }
    
    for(int i = 0; i < num_partitions_A_; i++) {
        matmul::Matrix matrix = matrix_partitions_[i]->matrix();
        uint64_t sub_matrix_num_rows = matrix.num_rows();
        uint64_t results_size = matrix.state().results_size();
        for(uint64_t j = 0; j < results_size; j++) {
            matmul::MatrixState_Result results = matrix.state().results(j);
            for(int k = 0; k < results.values_size(); k++) {
                double value = results.values(k);
                int64_t row = i * sub_matrix_num_rows + k / sub_matrix_num_rows;
                int64_t col = j * sub_matrix_num_rows + k % sub_matrix_num_rows;
                int64_t index = row * result_mat->num_cols() + col;

                spdlog::trace("Setting value {} at row = {}, col = {}, index = {}", value, row, col, index);

                result_mat->mutable_state()->set_values(index, value);
            }
        }
    }

    result_matrix_ = new Matrix<T>(result_mat);
}

template< typename T>
Matrix<T> &MatMul<T>::GetResultMatrix() {
    return *result_matrix_;
}

template< typename T>
Matrix<T> &MatMul<T>::GetInputMatrix() {
    matmul::Matrix *input_mat = new matmul::Matrix();
    input_mat->set_id(0);
    input_mat->set_num_rows(num_rows_);
    input_mat->set_num_cols(num_cols_);
    
    for(int i = 0; i < num_partitions_A_; i++) {
        matmul::Matrix matrix = matrix_partitions_[i]->matrix();

        for(int64_t j = 0; j < matrix.state().values_size(); j++) {
            double value = matrix.state().values(j);
            input_mat->mutable_state()->add_values(value);
        }
    }

    input_matrix_ = new Matrix<T>(input_mat);
    return *input_matrix_;
}

template< typename T>
void MatMul<T>::set_matmul_func(matmul_func_t matmul_func) {
    matmul_func_ = matmul_func;
}

template class MatMul<float>;
template class MatMul<double>;
#ifndef MATRIX_PROTO_H
#define MATRIX_PROTO_H

#include <iomanip>
#include <iostream>
#include <sstream>

#include <grpcpp/grpcpp.h>
#include "protos/matmul.grpc.pb.h"

template< typename T >
class Matrix {
public:
    Matrix(matmul::Matrix *matrix) {
        this->matrix_ = matrix;
    }

    int64_t get_size() const {
        return get_num_rows() * get_num_cols();
    }

    int64_t get_num_rows() const {
        return this->matrix_->num_rows();
    }

    int64_t get_num_cols() const {
        return this->matrix_->num_cols();
    }

    int64_t get_ld() const {
        return this->get_num_rows();
    }

    const T* get_data() const {
        return this->matrix_->state().values().data();
    }

    T* get_mutable_data() {
        return this->matrix_->mutable_state()->mutable_values()->mutable_data();
    }

    const T* get_results(int64_t index) const {
        return this->matrix_->state().results(index).values().data();
    }

    T* get_mutable_results(int64_t index) {
        return this->matrix_->mutable_state()->mutable_results(index)->mutable_values()->mutable_data();
    }

    T get_value(int64_t i, int64_t j) const {
        if (i < 0 || i >= get_num_rows()) throw std::invalid_argument("Matrix::get_value: i < 0 || i >= num_rows");
        if (j < 0 || j >= get_num_cols()) throw std::invalid_argument("Matrix::get_value: j < 0 || j >= num_cols");
        int64_t index = i*get_num_cols() + j;
        return this->matrix_->state().values(index);
    }

    void set_value(int64_t i, int64_t j, T value) {
        if (i < 0 || i >= get_num_rows()) throw std::invalid_argument("Matrix::set_value: i < 0 || i >= num_rows");
        if (j < 0 || j >= get_num_cols()) throw std::invalid_argument("Matrix::set_value: j < 0 || j >= num_cols");
        int64_t index = i*get_num_cols() + j;
        this->matrix_->mutable_state()->set_values(index, value);
    }

    std::string to_string() const {
        int64_t num_rows = get_num_rows();
        int64_t num_cols = get_num_cols();
        int max_width = calculate_width() + 2;
        
        std::stringstream ss;
        ss << "Size: " << num_rows << " x " << num_cols << std::endl;
        ss << std::setw(max_width + 1) << "";

        for (int64_t j = 0; j < num_cols; j++) {
            ss << std::setw(max_width - 1) << j << ": ";
        }
        
        ss << std::endl;

        for (int64_t i = 0; i < num_rows; i++) {
            for (int64_t j = 0; j < num_cols; j++) {
                if (j == 0) {
                    ss << std::setw(max_width - 1) << i << ": ";
                }
                ss << std::setw(max_width) << get_value(i, j) << " ";
            }
            ss << std::endl;
        }

        return ss.str();
    }

private:
    int calculate_width() const {
        int width = 0;
        int64_t num_rows = get_num_rows();
        int64_t num_cols = get_num_cols();
        
        for (int64_t i = 0; i < num_rows; i++) {
            for (int64_t j = 0; j < num_cols; j++) {
                std::stringstream ss;
                ss << get_value(i, j);
                width = std::max(width, (int) ss.str().size());
            }
        }
        return width;
    }

    matmul::Matrix *matrix_;
};

#endif // MATRIX_PROTO_H

#include "MatMul.h"

template< typename T>
MatMul<T>::MatMul(DistributedBuffer* buffer, std::string &input_file)
 : buffer_(buffer) , input_file_(input_file){
    auto init_interactions_func = [this]() {
        this->LoadInteractions();
    };
    auto init_partition_func = [this](partition::Partition *partition, int partition_start, int partition_end) {
        this->InitPartition(partition, partition_start, partition_end);
    };
    
    buffer_->Load(init_interactions_func, init_partition_func);
    buffer_->Start();
    buffer_->GetMatrixPartitions(matrix_partitions_);
}

template< typename T>
void MatMul<T>::LoadInteractions() {
    int64_t num_partitions = buffer_->GetNumPartitions();
    std::ifstream file(input_file_);
    if (!file.is_open()) throw std::invalid_argument("MatMul::read_from_file: file not found");
    file >> num_rows_ >> num_cols_;

    int64_t partition_size = (int)(ceil((double) num_rows_ / (double)num_partitions));
    spdlog::info("Partition size: {}", partition_size);
    
    data_.resize(num_rows_*num_cols_);

    for(int64_t i = 0; i< num_rows_*num_cols_; i++) {
        file >> data_[i];
    }

    buffer_->SetNumVertices(num_rows_);
    buffer_->SetPartitionSize(partition_size);
}

template< typename T>
void MatMul<T>::InitPartition(partition::Partition *partition, int partition_start, int partition_end) {
    int64_t num_partitions = buffer_->GetNumPartitions();

    matmul::MatrixPartition* matrix_partition = partition->mutable_matrix_partition();
    // for(int i = 0; i < num_partitions_A_; i++) {
    int64_t start_row = partition_start;
    int64_t end_row = partition_end;
    int64_t id = partition->partition_id();
    int64_t start_col = 0;
    int64_t end_col = num_cols_;    
    
    matmul::Matrix *matrix = matrix_partition->mutable_matrix();
    matrix->set_id(id);
    matrix->set_num_rows(end_row - start_row);
    matrix->set_num_cols(end_col - start_col);
    
    // Data is stored sequentially in vector data. Note: End row is exclusive
    int64_t start_index = start_row * num_cols_ + start_col;
    int64_t end_index = (end_row - 1) * num_cols_ + end_col;

    for(int64_t j = start_index; j < end_index; j++) {
        matrix->mutable_state()->add_values(data_[j]);
    }
    
    int64_t row_range = (end_row - start_row) * (end_row - start_row);
    for(int k = 0; k < num_partitions; k++) {
        matmul::MatrixState_Result *results = matrix->mutable_state()->add_results();
        for(int64_t j = 0; j < row_range; j++) {
            results->add_values(0);
        }
    }
    spdlog::debug("Matrix Partition {} has a Matrix of Size: {} X {}", id, matrix->num_rows(), matrix->num_cols());
    // }
}



template< typename T>
void MatMul<T>::initialize() {
    buffer_->GetMatrixPartitions(matrix_partitions_);
}

template< typename T>
void MatMul<T>::startProcessing(int num_iters) {
    for (int iter = 0; iter < num_iters; iter++) {
        spdlog::debug("Iteration: {}", iter);
        buffer_->InitEpoch();
        while(true){
            std::optional<WorkUnit> opt_interaction = buffer_->GetWorkUnit();
            if(opt_interaction == std::nullopt){
                spdlog::debug("Matrix multiplication complete.");
                break;
            }
            
            WorkUnit interaction = *opt_interaction;
            partition::Partition *src_partition = interaction.src();
            partition::Partition *dst_partition = interaction.dst();
            spdlog::trace("Processing interaction: src: {}, dst: {}", src_partition->partition_id(), dst_partition->partition_id());

            matmul::MatrixPartition* src = src_partition->mutable_matrix_partition();
            matmul::MatrixPartition* dst = dst_partition->mutable_matrix_partition();
            processInteraction(src, dst);
            buffer_->MarkInteraction(interaction);
        }

        buffer_->WaitForEpochCompletion();
    }
}

template< typename T>
void MatMul<T>::processInteraction(matmul::MatrixPartition *src_partition, matmul::MatrixPartition *dst_partition) {
    matmul::Matrix *matrix_A = src_partition->mutable_matrix();
    matmul::Matrix *matrix_B = dst_partition->mutable_matrix();
    int64_t dst_partition_id = matrix_B->id();

    Matrix<T> matrix_A_proto(matrix_A);
    Matrix<T> matrix_B_proto(matrix_B);
    matmul_func_(matrix_A_proto, matrix_B_proto, dst_partition_id);
}

template< typename T>
void MatMul<T>::collectResults() {
    buffer_->GetMatrixPartitions(matrix_partitions_);

    matmul::Matrix *result_mat = new matmul::Matrix();
    result_mat->set_id(0);

    result_mat->set_num_rows(num_rows_);
    result_mat->set_num_cols(num_rows_);

    for(int64_t i = 0; i < num_rows_ * num_rows_; i++) {
        result_mat->mutable_state()->add_values(0);
    }
    
    for(size_t i = 0; i < matrix_partitions_.size(); i++) {
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
    int total_size = 0;
    
    for(size_t i = 0; i < matrix_partitions_.size(); i++) {
        matmul::Matrix matrix = matrix_partitions_[i]->matrix();

        for(int64_t j = 0; j < matrix.state().values_size(); j++) {
            double value = matrix.state().values(j);
            input_mat->mutable_state()->add_values(value);
            total_size++;
        }
    }

    int num_rows = total_size / num_cols_;

    input_mat->set_num_rows(num_rows);
    input_mat->set_num_cols(num_cols_);

    input_matrix_ = new Matrix<T>(input_mat);
    return *input_matrix_;
}

template< typename T>
void MatMul<T>::set_matmul_func(matmul_func_t matmul_func) {
    matmul_func_ = matmul_func;
}

template class MatMul<float>;
template class MatMul<double>;
#ifndef BASE_APP_MATMUL_H
#define BASE_APP_MATMUL_H

#include "src/matmul/lib/MatMul.h"

template <typename T>
class BaseMatMulApp : public MatMul<T> {
protected:
    BaseMatMulApp(DistributedBuffer* buffer, std::string &input_file): MatMul<T>(buffer, input_file) {}

///////////////////////// Static Methods to be implemented ///////////////////////////////
//  static void matmul(Matrix<T> &matrix_A, Matrix<T> &matrix_B, int64_t result_idx)
//  static variables (if any)
///////////////////////////////////////////////////////////////////////////////////


//////////////////////////////// APIs available ///////////////////////////////////
//  void set_matmul_func(matmul_func);
//  void initialize();
//  void startProcessing(const int &num_iters);
//  void collectResults();
//  Matrix<T> &GetResultMatrix();     
//  Matrix<T> &GetInputMatrix();      
//
//  Note: For Matrix APIs, see Matrix.h         
///////////////////////////////////////////////////////////////////////////////////

};

#endif // BASE_APP_MATMUL
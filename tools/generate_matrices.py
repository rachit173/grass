# Usage:
# python3 generate_matrices.py <m> <n> <k>

import sys
import os
import uuid
import random

BASE_DIR = "/mnt/Work/grass/resources/matmul/"

def create_matrix(m, n):
    matrix = [[random.randint(1, 100) for i in range(n)] for j in range(m)]
    return matrix

def split_list(lst, n):
    k, m = divmod(len(lst), n)
    return [lst[i*k+min(i, m):(i+1)*k+min(i+1, m)] for i in range(n)]

def read_matrix(filename):
    matrix = []
    with open(filename, 'r') as f:
        m, n = f.readline().split()
        for line in f:
            row_mat = split_list(line.split(), int(m))
            for row in row_mat:
                matrix.append([int(x) for x in row])
    return matrix

def write_matrix(filename, matrix, formatted = False):
    with open(filename, 'w') as f:
        f.write(str(len(matrix)) + " " + str(len(matrix[0])) + "\n")
        if formatted == False:
            f.write(" ".join([str(x) for row in matrix for x in row]))
        else:
            for row in matrix:
                f.write(" ".join([str(x) for x in row]) + "\n")


def multiply_matrices(matA, matB):
    m = len(matA)
    n = len(matB[0])
    p = len(matB)
    matC = [[0 for i in range(n)] for j in range(m)]
    for i in range(m):
        for j in range(n):
            for k in range(p):
                matC[i][j] += matA[i][k] * matB[k][j]
    return matC

def transpose_matrix(matrix):
    return [[matrix[j][i] for j in range(len(matrix))] for i in range(len(matrix[0]))]

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: generate_matrices.py <m> <n>")
        sys.exit(1)

    m = int(sys.argv[1])
    n = int(sys.argv[2])

    input_matrix_filename = os.path.join(BASE_DIR, "input_matrix.txt")
    result_matrix_filename = os.path.join(BASE_DIR, "result_matrix.txt")

    input_matrix = create_matrix(m, n)
    matB = transpose_matrix(input_matrix)
    result_matrix = multiply_matrices(input_matrix, matB)
    
    print("Writing Input Matrix to {}".format(input_matrix_filename))
    write_matrix(input_matrix_filename, input_matrix)
    print("Writing Result Matrix to {}".format(result_matrix_filename))
    write_matrix(result_matrix_filename, result_matrix, True)



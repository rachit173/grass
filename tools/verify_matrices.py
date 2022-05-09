# Usage:
# python3 verify_matrices.py <input_prefix>

import sys

BASE_DIR = "/mnt/Work/grass/resources/matmul/"

if len(sys.argv) < 2:
    print("Usage: verify_matrices.py <input_prefix>")
    sys.exit(1)

input_prefix=sys.argv[1]

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

actual_result_file = "actual_results/" + input_prefix + "_input.txt_0"
expected_result_file = input_prefix + "_output.txt"

actual_result = read_matrix(BASE_DIR + actual_result_file)
expected_result = read_matrix(BASE_DIR + expected_result_file)

assert len(actual_result) == len(expected_result)
assert len(actual_result[0]) == len(expected_result[0])

for i in range(len(actual_result)):
    for j in range(len(actual_result[0])):
        if actual_result[i][j] != expected_result[i][j]:
            print("Error: At {}, {}: Expected {}, Got {}".format(i, j, expected_result[i][j], actual_result[i][j]))
            sys.exit(1)

print("Matrices are equal!")
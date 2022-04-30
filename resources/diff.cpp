#include <bits/stdc++.h>
using namespace std;

int main(int argc, char** argv) {
    if(argc < 5) {
        cout << "Usage: " << argv[0] << " <base_dir> <application> <test filename> <tolerance>" << endl;
        return 0;
    }
    string base_dir = argv[1];
    string application = argv[2];
    string filename = argv[3];
    double tolerance = atof(argv[4]);
    string expected_results_filepath = base_dir + application + "/expected_results/" + filename;
    string actual_results_filepath = base_dir + application + "/actual_results/" + filename;

    ifstream expected_results_file(expected_results_filepath);
    ifstream actual_results_file(actual_results_filepath);

    string expected_result_line;
    string actual_result_line;

    bool eof_file1 = false, eof_file2 = false;

    while (true) {
        // Compare files line by line and get diff with tolerance for numbers
        if(!eof_file1) getline(actual_results_file, actual_result_line);
        if(!eof_file2) getline(expected_results_file, expected_result_line);

        if (expected_results_file.eof()) eof_file2 = true;
        if (actual_results_file.eof()) eof_file1 = true;

        if (eof_file1 && eof_file2) break;

        if(eof_file1) {
            cout << "Actual results file is shorter than expected results file" << endl;
            cout << expected_result_line << endl;
            while(!expected_results_file.eof()){
                getline(expected_results_file, expected_result_line);
                cout << expected_result_line << endl;
            }
            break;
        }

        if(eof_file2) {
            cout << "Expected results file is shorter than actual results file" << endl;
            cout << actual_result_line << endl;
            while(!actual_results_file.eof()){
                getline(actual_results_file, actual_result_line);
                cout << actual_result_line << endl;
            }
            break;
        }

        stringstream ss1(actual_result_line), ss2(expected_result_line);
        double actual_result, expected_result;
        int64_t actual_vertex_id, expected_vertex_id;
        ss1 >> actual_vertex_id >> actual_result;
        ss2 >> expected_vertex_id >> expected_result;

        if (actual_vertex_id != expected_vertex_id) {
            cout << "Vertex ids don't match" << endl;
            cout << "Actual vertex id: " << actual_vertex_id << endl;
            cout << "Expected vertex id: " << expected_vertex_id << endl;
        }

        if (abs(actual_result - expected_result) > tolerance) {
            cout << "Results don't match" << endl;
            cout << "Actual result: " << actual_result << endl;
            cout << "Expected result: " << expected_result << endl;
        }
    }
}
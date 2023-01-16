#include <bits/stdc++.h>
#include <dirent.h>
#include <errno.h>
using namespace std;

// extract the first word from each line and sort using it
bool comp(const string &a, const string &b) {
    return stoi(a.substr(0, a.find(" "))) < stoi(b.substr(0, b.find(" ")));
};

void read_to_vector_and_sort(string input_file_path, vector<string> &results) {
    ifstream input_file(input_file_path);
    if (!input_file.is_open()) {
        cout << "Error opening file: " << input_file_path << ". Exiting..." << endl;
        exit(1);
    }
    string line;
    while (getline(input_file, line)) {
        results.push_back(line);
    }
    sort(results.begin(), results.end(), comp);
}

void write_to_sorted_file(string input_file_path, vector<string> &results) {
    ofstream output_file(input_file_path);
    if (!output_file.is_open()) {
        cout << "Error opening file: " << input_file_path << ". Exiting..." << endl;
        exit(1);
    }
    for (auto &result : results) {
        output_file << result << endl;
    }
    output_file.close();
}

vector<string> get_partition_filenames(string dir, string reqd_filename) {
    DIR *dp;
    struct dirent *dirp;
    vector<string> files = {};
    if((dp = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        exit(1);
    }
    while ((dirp = readdir(dp)) != NULL) {
        string filename = dirp->d_name;
        if(filename.find(reqd_filename + "_") != string::npos && filename.find("_sorted") == string::npos) {
            filename = dir + "/" + filename;
            files.push_back(filename);
        }
    }
    closedir(dp);
    return files;
}

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
    string actual_results_filepath_sorted = actual_results_filepath + "_sorted";

    vector<string> expected_results, actual_results;
    vector<string> partition_filenames = get_partition_filenames(base_dir + application + "/actual_results/", filename);
    for(auto partition_filename: partition_filenames){
        vector<string> tmp = {};
        read_to_vector_and_sort(partition_filename, tmp);
        actual_results.insert(actual_results.end(), tmp.begin(), tmp.end());
    }
    sort(actual_results.begin(), actual_results.end(), comp);
    read_to_vector_and_sort(expected_results_filepath, expected_results);
    write_to_sorted_file(actual_results_filepath_sorted, actual_results);

    string expected_result_line;
    string actual_result_line;

    bool eof_file1 = false, eof_file2 = false;
    int line_number = 0;
    int expected_results_size = expected_results.size(), actual_results_size = actual_results.size();

    cout << "Expected results size: " << expected_results_size << endl;
    cout << "Actual results size: " << actual_results_size << endl;

    if(expected_results_size < actual_results_size) {
        cout << "Actual results file has more lines than expected results file" << endl;
    } else if(expected_results_size > actual_results_size) {
        cout << "Actual results file has less lines than expected results file" << endl;
    }
    
    while (line_number < min(expected_results_size, actual_results_size)) {
        // Compare files line by line and get diff with tolerance for numbers
        expected_result_line = expected_results[line_number];
        actual_result_line = actual_results[line_number];

        stringstream ss1(actual_result_line), ss2(expected_result_line);
        double actual_result, expected_result;
        int64_t actual_vertex_id, expected_vertex_id;
        ss1 >> actual_vertex_id >> actual_result;
        ss2 >> expected_vertex_id >> expected_result;

        if (actual_vertex_id != expected_vertex_id) {
            cout << "Vertex ids don't match" << "Actual vertex id: " << actual_vertex_id << " Expected vertex id: " << expected_vertex_id << endl;
        }

        if (abs(actual_result - expected_result) > tolerance) {
            cout << "Results don't match" << "Actual result: " << actual_result << " Expected result: " << expected_result << endl;
        }

        line_number++;
    }
}
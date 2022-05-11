#ifndef METRICS_H
#define METRICS_H

#include<bits/stdc++.h>

using namespace std;

class Metrics {
private:
    double max, min, sum, count, stddev;
    std::vector<double> data;
public:
    Metrics() {
        max = 0;
        min = 0;
        count = 0;
        stddev = 0;
        sum = 0;
        data.clear();
    }

    void reset() {
        max = 0;
        min = 0;
        count = 0;
        stddev = 0;
        sum = 0;
        data.clear();
    }

    void add(double value) {
        if (count == 0) {
            min = value;
            max = value;
        }
        if (value > max) {
            max = value;
        }
        if (value < min) {
            min = value;
        }
        sum += value;
        count++;
        data.push_back(value);
    }

    double get_max() {
        return max;
    }

    double get_min() {
        return min;
    }

    double get_mean() {
        return sum / count;
    }

    double get_sum() {
        return sum;
    }

    long get_count() {
        return count;
    }

    double get_stddev(){
        double mean = get_mean();
        for (int i = 0; i < (int)data.size(); i++) {
            stddev += pow(data[i] - mean, 2);
        }
        stddev = sqrt(stddev / data.size());
        return stddev;
    }

    double get_median() {
        std::sort(data.begin(), data.end());
        if (data.size() % 2 == 0) {
            return (data[data.size() / 2] + data[data.size() / 2 - 1]) / 2;
        } else {
            return data[data.size() / 2];
        }
    }

    void pretty_print() {
        std::cout << "Min: " << get_min() << std::endl;
        std::cout << "Max: " << get_max() << std::endl;
        std::cout << "Mean: " << get_mean() << std::endl;
        std::cout << "Median: " << get_median() << std::endl;
        std::cout << "Stddev: " << get_stddev() << std::endl;
    }

    static string get_header() {
        return "Count,Sum(ms),Min(ms),Max(ms),Mean(ms),Median(ms),Stddev(ms)";
    }

    string get_metrics_in_ms() {
        string metrics_str = "";
        metrics_str +=  to_string(get_count())
                        + "," +to_string(get_sum()/1e6)
                        + "," + to_string(get_min()/1e6) 
                        + "," + to_string(get_max()/1e6)
                        + "," + to_string(get_mean()/1e6) 
                        + "," + to_string(get_median()/1e6)
                        + "," + to_string(get_stddev()/1e6);
        return metrics_str;
    }
    
};


extern Metrics overhead_metrics;
#endif
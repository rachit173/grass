#ifndef PERF_UTILS_H
#define PERF_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <chrono>


class PerfUtils {
  public:
    PerfUtils(int rank) : rank_(rank) {}
    void StartTime() {
      start_ = std::chrono::steady_clock::now();
    }
    void AddStageConnection(const std::string &prev_stage, const std::string &stage) {
      stages_outgoing_edges_[prev_stage].push_back(stage);
    }
    void LogPoint(const std::string& stage, const std::string& round) {
      auto point = std::chrono::steady_clock::now();
      double tm = std::chrono::duration_cast<std::chrono::nanoseconds>(point - start_).count();
      points_[stage].push_back({round, tm});
    }
  private:
  std::chrono::_V2::steady_clock::time_point start_;
  std::map<std::string, std::vector<std::string>> stages_outgoing_edges_;
  std::map<std::string, std::vector<std::pair<std::string, double>>> points_;
  int rank_; 
};

#endif // PERF_UTILS_H
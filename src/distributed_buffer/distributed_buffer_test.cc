#include "distributed_buffer.h"

#include "gtest/gtest.h"
#include <thread>
using namespace std;
typedef vector<vector<pair<int, int>>> vvii;
TEST(DistributedBufferTest, CheckServersSetup) {
  // std::thread t1([&]() {
  //   DistributedBufferConfig config;
  //   config.self_rank = 0;
  //   config.num_partitions = 8;
  //   config.capacity = 4;
  //   config.num_workers = 2;
  //   config.server_addresses = {"localhost:50051", "localhost:50052"};
  //   DistributedBuffer buffer(config);
  // });
  // std::thread t2([&]() {
  //   DistributedBufferConfig config;
  //   config.self_rank = 1;
  //   config.num_partitions = 8;
  //   config.capacity = 4;
  //   config.num_workers = 2;
  //   config.server_addresses = {"localhost:50051", "localhost:50052"};
  //   DistributedBuffer buffer(config);
  // });  
  // t1.join();
  // t2.join();
}

void PrintMatchings(const vvii& matchings) {
  for (int  i = 0; i < matchings.size(); i++) {
    std::cout << "Round " << i << ": ";
    for (int j = 0; j < matchings[i].size(); j++) {
      std::cout << "(" << matchings[i][j].first << ", " << matchings[i][j].second << ") ";
    }
    std::cout << "\n";
  }
}

bool VerifyMatchings(uint32_t m, const vvii& matchings) {
  if (matchings.size() != m-1) return false;

  for (const auto& matching : matchings) {
    if (matching.size() != m/2) return false;
    set<int> vertices;
    for (auto edge: matching) {
      vertices.insert(edge.first);
      vertices.insert(edge.second);
    }
    if (vertices.size() != m) return false;
  }
  set<pair<int, int>> edges;
  for (auto& matching: matchings) {
    for (auto& edge: matching) {
      edges.insert(edge);
    }
  }
  int num_total_edges = (m * (m-1))/2;
  if (edges.size() != num_total_edges) {
    return false;
  }
  return true;
}

bool VerifyPlan(uint32_t m, const vvii& machine_state) {
  if (!VerifyMatchings(m, machine_state)) return false;
  for (int i = 1; i < machine_state.size(); i++) {
    for (int j = 0; j < machine_state[i].size(); j++) {
      bool first_match = (machine_state[i][j].first == machine_state[i-1][j].first) 
                          || (machine_state[i][j].first == machine_state[i-1][j].second);
      bool second_match = (machine_state[i][j].second == machine_state[i-1][j].first) 
                          || (machine_state[i][j].second == machine_state[i-1][j].second);
      bool only1_match = (first_match ^ second_match);
      if(!only1_match) return false;
    }
  }
  return true;
}

TEST(DistributedBufferTest, GenerateMatchings3) {
  vvii matchings;
  int k = 3;
  int n = 2*k;
  GenerateMatchings(0, n, matchings);
  PrintMatchings(matchings);
  EXPECT_EQ(VerifyMatchings(n, matchings), true);
}


TEST(DistributedBufferTest, GenerateMatchingsK2) {
  vvii matchings;
  int k = 2;
  int n = 2*k;
  GenerateMatchings(0, n, matchings);
  PrintMatchings(matchings);
  EXPECT_EQ(VerifyMatchings(n, matchings), true);
}

TEST(DistributedBufferTest, GenerateMatchingsK4) {
  vvii matchings;
  int k = 4;
  int n = 2*k;
  GenerateMatchings(0, n, matchings);
  PrintMatchings(matchings);
  EXPECT_EQ(VerifyMatchings(n, matchings), true);
}

TEST(DistributedBufferTest, GenerateMatchingsK5) {
  vvii matchings;
  int k = 5;
  int n = 2*k;
  GenerateMatchings(0, n, matchings);
  PrintMatchings(matchings);
  EXPECT_EQ(VerifyMatchings(n, matchings), true);
}

TEST(DistributedBufferTest, GenerateMatchingsK104) {
  vvii matchings;
  int k = 104;
  int n = 2*k;
  GenerateMatchings(0, n, matchings);
  PrintMatchings(matchings);
  EXPECT_EQ(VerifyMatchings(n, matchings), true);
}

void PrintPlan(const vector<vector<pair<int, int>>>& plan) {
  for (int i = 0; i < plan[0].size(); i++) {
    cout << "Machine: " << i << "\n";
    for (int j = 0; j < plan.size(); j++) {
      cout << "Get " << plan[j][i].second << " from " << plan[j][i].first << "\n";
    }
  }
}

void PrintMachineState(const vvii& machine_state) {
  for (int i = 0; i < machine_state.size(); i++) {
    cout << "Round " << i << ": ";
    for (int j = 0; j < machine_state[i].size(); j++) {
      cout << "(" << machine_state[i][j].first << ", " << machine_state[i][j].second << "), ";
    }
    cout << "\n";
  }
}
TEST(DistributedBufferTest, GeneratePlanK3) {
  vvii matchings;
  int k = 3;
  int n = 2*k;
  GenerateMatchings(0, n, matchings);
  PrintMatchings(matchings);
  // EXPECT_EQ(VerifyMatchings(n, matchings), true);
  vvii plan;
  vvii machine_state;
  GeneratePlan(matchings, plan, machine_state);
  PrintPlan(plan);
  PrintMachineState(machine_state);
  EXPECT_EQ(VerifyPlan(n, machine_state), true);
}

TEST(DistributedBufferTest, GeneratePlanK104) {
  vvii matchings;
  int k = 104;
  int n = 2*k;
  GenerateMatchings(0, n, matchings);
  PrintMatchings(matchings);
  // EXPECT_EQ(VerifyMatchings(n, matchings), true);
  vvii plan;
  vvii machine_state;
  GeneratePlan(matchings, plan, machine_state);
  PrintPlan(plan);
  PrintMachineState(machine_state);
  EXPECT_EQ(VerifyPlan(n, machine_state), true);
}

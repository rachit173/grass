#include "distributed_buffer.h"

#include "gtest/gtest.h"
#include <thread>
using namespace std;

TEST(DistributedBufferTest, CheckServersSetup) {
  std::thread t1([&]() {
    DistributedBufferConfig config;
    config.self_rank = 0;
    config.num_partitions = 8;
    config.capacity = 4;
    config.num_workers = 2;
    config.server_addresses = {"localhost:50051", "localhost:50052"};
    DistributedBuffer buffer(config);
  });
  std::thread t2([&]() {
    DistributedBufferConfig config;
    config.self_rank = 1;
    config.num_partitions = 8;
    config.capacity = 4;
    config.num_workers = 2;
    config.server_addresses = {"localhost:50051", "localhost:50052"};
    DistributedBuffer buffer(config);
  });  
  t1.join();
  t2.join();
}
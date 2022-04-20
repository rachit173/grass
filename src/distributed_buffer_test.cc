#include "distributed_buffer.h"

#include "gtest/gtest.h"


TEST(DistributedBufferTest, GenerateMatchings) {
  std::string server_address = "localhost:50051";
  DistributedBuffer buffer(0, 8, 4, 2, server_address);
}
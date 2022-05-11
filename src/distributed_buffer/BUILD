load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
package(default_visibility = ["//visibility:public"])

cc_library(
  name = "distributed_buffer",
  srcs = [
    "distributed_buffer.cc",
    "common.cc",
    "buffer_init_interface.cc",
    "partition_service.cc",
    "populate_partition.cc",
    "matching_generator.cc",
  ],
  hdrs = [
    "distributed_buffer.h",
    "matching_generator.h",
  ],
  deps = [
    "//protos:partition_cc_grpc",
    "//src/utils:metrics",
    "@com_github_grpc_grpc//:grpc++",
    "@com_google_absl//absl/strings",
    "@com_github_grpc_grpc//:grpc++_reflection",
    "@spdlog",
  ],
  copts = [
    "-std=c++17"
  ],
  linkopts = [
    "-lpthread"
  ]
)

cc_test(
  name = "distributed_buffer_test",
  srcs = ["distributed_buffer_test.cc"],
  deps = [
    ":distributed_buffer",
    "@com_google_googletest//:gtest_main",
    "//src/apps:page_rank",
    "//src/apps:connected_comp",
    "//src/apps:shortest_path",
    "//src/apps:degree",
  ],
  copts = [
    "-std=c++17"
  ],
  linkopts = [
    "-lpthread"
  ]
)
package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")


proto_library(
  name = "graph_proto",
  srcs = ["graph.proto"]
)

proto_library(
  name = "matmul_proto",
  srcs = ["matmul.proto"]
)

proto_library(
  name = "partition_proto",
  srcs = ["partition.proto"],
  deps = [":graph_proto", ":matmul_proto"],
)

cc_proto_library(
  name = "partition_cc_proto",
  deps = [":partition_proto"],
)

cc_grpc_library(
  name = "partition_cc_grpc",
  srcs = [":partition_proto"],
  grpc_only = True,
  deps = [":partition_cc_proto"],
)

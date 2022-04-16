package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")


proto_library(
  name = "graph_proto",
  srcs = ["graph.proto"],
  deps = ["@com_google_protobuf//:any_proto"],
)

cc_proto_library(
  name = "graph_cc_proto",
  deps = [":graph_proto"],
)

cc_grpc_library(
  name = "graph_cc_grpc",
  srcs = [":graph_proto"],
  grpc_only = True,
  deps = [":graph_cc_proto"],
)
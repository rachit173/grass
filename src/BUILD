load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
  name = "distributed_buffer",
  srcs = ["distributed_buffer.cc"],
  hdrs = ["distributed_buffer.h"],
  deps = [
    "//protos:graph_cc_grpc",
    "@com_github_grpc_grpc//:grpc++",
    "@com_google_absl//absl/strings",
    "@com_github_grpc_grpc//:grpc++_reflection",
  ],
  copts = [
    "-std=c++17"
  ],
  linkopts = [
    "-lpthread"
  ]
)

cc_binary(
  name = "run_app",
  srcs = [ "run_app.cc",],
  deps = [
    "//src/apps:page_rank",
    "//src/apps:connected_comp",
    "//src/apps:shortest_path",
  ]
)

cc_binary(
  name = "worker",
  srcs = ["worker_main.cc"],
  deps = [
    "//protos:graph_cc_grpc",
    "@com_github_grpc_grpc//:grpc++",
    "@com_google_absl//absl/strings",
    "@com_github_grpc_grpc//:grpc++_reflection",
    "apps_page_rank"
  ],
  copts = [
    "-std=c++17"
  ],
  linkopts = [
    "-lpthread"
  ]
)


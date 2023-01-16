load("@rules_cc//cc:defs.bzl", "cc_binary")
package(default_visibility = ["//visibility:public"])

cc_library(
  name = "base_lib",
  srcs = ["base_lib.h"],
  deps = [ 
    "//protos:partition_cc_grpc",
    "@com_github_grpc_grpc//:grpc++",
    "@spdlog"
  ],
  copts = [
    "-std=c++17"
  ],
)

cc_binary(
  name = "run_app",
  srcs = [ "run_app.cc"],
  deps = [
    "//src/apps:page_rank",
    "//src/apps:connected_comp",
    "//src/apps:shortest_path",
    "//src/apps:degree",
    "//src/utils:logger",
    "//src/utils:config_utils",
  ],
  copts = [
    "-std=c++17"
  ],
)

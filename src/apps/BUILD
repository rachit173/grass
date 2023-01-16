load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
  name = "page_rank",
  srcs = ["page_rank.cc"],
  hdrs = ["base_app.h", "page_rank.h"],
  deps = [
    "//protos:partition_cc_grpc",
    "//src/graph:graph",
  ],
  copts = [
    "-std=c++17"
  ],
)

cc_library(
  name = "shortest_path",
  srcs = ["shortest_path.cc"],
  hdrs = ["base_app.h", "shortest_path.h"],
  deps = [
    "//protos:partition_cc_grpc",
    "//src/graph:graph",
  ],
  copts = [
    "-std=c++17"
  ],
)

cc_library(
  name = "connected_comp",
  srcs = ["connected_comp.cc"],
  hdrs = ["base_app.h", "connected_comp.h"],
  deps = [
    "//protos:partition_cc_grpc",
    "//src/graph:graph",
  ],
  copts = [
    "-std=c++17"
  ],
)

cc_library(
  name = "degree",
  srcs = ["degree.cc"],
  hdrs = ["base_app.h", "page_rank.h", "degree.h"],
  deps = [
    "//protos:partition_cc_grpc",
    "//src/graph:graph",
    ":page_rank",
  ],
  copts = [
    "-std=c++17"
  ],
)

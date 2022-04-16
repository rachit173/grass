load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
  name = "page_rank",
  srcs = ["page_rank.cc"],
  hdrs = ["base_app.h", "page_rank.h"],
  deps = [
    "//protos:graph_cc_grpc",
    "//src/graph:graph",
  ]
)

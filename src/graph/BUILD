load("@rules_cc//cc:defs.bzl", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "graph",
    srcs = ["Graph.cc"],
    hdrs = ["Vertex.h", "Edge.h", "Graph.h"],
    deps = [ 
      "//protos:graph_cc_grpc",
      "@com_github_grpc_grpc//:grpc++",
      "@spdlog",
      "//src/distributed_buffer:distributed_buffer"
    ],
)

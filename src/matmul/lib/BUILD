load("@rules_cc//cc:defs.bzl", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "matmul",
    srcs = ["MatMul.cc"],
    hdrs = ["Matrix.h", "MatMul.h"],
    linkstatic = 1,
    deps = [
        "//protos:partition_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@spdlog",
        "//src/distributed_buffer:distributed_buffer"
    ],
    copts = [
      "-std=c++17"
    ]
)

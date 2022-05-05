load("@rules_cc//cc:defs.bzl", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "matmul",
    srcs = ["MatMul.cc"],
    hdrs = ["Matrix.h", "MatMul.h"],
    linkstatic = 1,
    deps = [
        "//protos:matmul_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@spdlog"]
)

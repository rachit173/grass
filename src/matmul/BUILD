
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "run_app",
    srcs = ["run_app.cc"], 
    deps = [
        "//src/matmul/apps:attention_matmul",
        "@spdlog"],
    linkopts = ["-lpthread"],
)

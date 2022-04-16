load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_binary(
  name = "run_app",
  srcs = [ "run_app.cc",],
  deps = [
    "//src/apps:page_rank",
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


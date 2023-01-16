load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
  name = "logger",
  srcs = ["logger.h"],
  deps = [
    "@spdlog"
  ]
)

cc_library(
  name = "config_utils",
  srcs = ["config_utils.h"]
)


cc_library(
  name = "perf_utils",
  srcs = ["perf_utils.h"]
)

cc_library(
  name = "metrics",
  srcs = ["timer.h", "metrics.h"]
)


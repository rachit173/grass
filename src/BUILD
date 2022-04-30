load("@rules_cc//cc:defs.bzl", "cc_binary")
package(default_visibility = ["//visibility:public"])

cc_binary(
  name = "run_app",
  srcs = [ "run_app.cc", "utils/config_utils.h"],
  deps = [
    "//src/apps:page_rank",
    "//src/apps:connected_comp",
    "//src/apps:shortest_path",
    "//src/apps:degree",
  ],
  copts = [
    "-std=c++17"
  ],
)

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
  name = "attention_matmul",
  srcs = ["attention_matmul.cc"],
  hdrs = ["base_matmul.h", "attention_matmul.h"],
  deps = [
    "@lapackpp//:lapackpp",
    "//src/matmul/lib:matmul",
  ]
)


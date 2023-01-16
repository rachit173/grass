load("@rules_cc//cc:defs.bzl", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "spdlog",
    srcs = glob(["src/**/*.c*"]),
    hdrs = glob(["include/**/*.h*"]),
    defines = ["SPDLOG_COMPILED_LIB"],
    includes = ["include"],
)
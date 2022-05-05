package(
    default_visibility = ["//visibility:public"]
)

include_files = [
    "include/**/*.h",
    "include/**/*.hh",
]

lib_files = [
    "lib/**/*.so",
]

cc_library(
    name = "lapackpp",
    srcs = glob(lib_files),
    hdrs = glob(include_files),
    includes=["include"],
    linkstatic = 1,
)

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "geometry",
    srcs = ["core/geometry.cc"],
    hdrs = ["core/geometry.h"],
)

cc_library(
    name = "trajectory_builder",
    srcs = ["core/trajectory_builder.cc"],
    hdrs = ["core/trajectory_builder.h"],
    deps = ["@hyw_proto//proto/sim:runtime_cc_proto"],
)

cc_library(
    name = "planner_core",
    srcs = ["core/planner.cc"],
    hdrs = ["core/planner.h"],
    deps = [
        ":geometry",
        ":trajectory_builder",
        "@hyw_proto//proto/sim:runtime_cc_proto",
    ],
)

cc_library(
    name = "session_manager",
    srcs = ["service/session_manager.cc"],
    hdrs = ["service/session_manager.h"],
    deps = [":planner_core"],
)

cc_library(
    name = "planner_service",
    srcs = ["service/planner_service_impl.cc"],
    hdrs = ["service/planner_service_impl.h"],
    deps = [
        ":session_manager",
        "@hyw_proto//proto/planner:planner_cc_grpc",
    ],
)

cc_binary(
    name = "planner_server",
    srcs = ["planner_server_main.cc"],
    deps = [
        ":planner_service",
        "@com_github_grpc_grpc//:grpc++",
    ],
)

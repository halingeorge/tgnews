local_repository(
  # Name of the Abseil repository. This name is defined within Abseil's
  # WORKSPACE file, in its `workspace()` metadata
  name = "com_google_absl",

  # NOTE: Bazel paths must be absolute paths. E.g., you can't use ~/Source
  path = "third_party/abseil",
)

local_repository(
    name = "com_google_googletest",
    path = "third_party/gtest/github",
)

bind(
  name = "gtest",
  actual = "@com_google_googletest//:gtest",
)

bind(
  name = "gtest_main",
  actual = "@com_google_googletest//:gtest_main",
)

local_repository(
  name = "com_google_glog",
  path = "third_party/glog/upstream",
)

bind(
  name = "glog",
  actual = "@com_google_glog//:glog",
)

new_local_repository(
  name = "com_github_gflags_gflags",
  path = "third_party/gflags/upstream",
  build_file = "third_party/gflags/upstream/BUILD",
)

# So gflags can be accessed via //external:gflags.
bind(
  name = "gflags",
  actual = "@com_github_gflags_gflags//:gflags",
)
#include "server/server.h"

#include "gflags/gflags.h"
#include "glog/logging.h"

#include <iostream>

DEFINE_string(server_hostname, "localhost", "server hostname");
DEFINE_int32(server_port, 12345, "server port");
DEFINE_bool(log_to_stderr, false, " log to stderr");
DEFINE_string(content_path, "content", "content path");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  FLAGS_logtostderr = FLAGS_log_to_stderr;

  google::InitGoogleLogging(argv[0]);

  return 0;
}

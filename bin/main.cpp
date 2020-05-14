#include "server/server.h"

#include "gflags/gflags.h"
#include "glog/logging.h"

#include <iostream>

DEFINE_bool(log_to_stderr, false, "Listening port");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  FLAGS_logtostderr = FLAGS_log_to_stderr;

  google::InitGoogleLogging(argv[0]);

  tgnews::Server server;
  server.Run();

  return 0;
}

#include "server/server.h"

#include "absl/flags/flag.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include <iostream>

DEFINE_int32(port, 10000, "Listening port");
DEFINE_bool(log_to_stderr, false, "Listening port");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  absl::SetFlag(&FLAGS_logtostderr, FLAGS_log_to_stderr);

  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "starting server\n";

  std::cout << "cout: starting server: " << FLAGS_port << std::endl;

  tgnews::Server server(FLAGS_port);

  return 0;
}

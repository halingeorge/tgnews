#include "server/server.h"

#include "gflags/gflags.h"
#include "glog/logging.h"

#include <iostream>

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  LOG(ERROR) << "starting server\n";

  std::cout << "cout: starting server" << std::endl;

  tgnews::Server server(101);

  return 0;
}
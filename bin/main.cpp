#include "server/server.h"
#include "server/file_cache.h"
#include "server/context.h"


#include "gflags/gflags.h"
#include "glog/logging.h"

#include <iostream>

DEFINE_int32(port, 10000, "Listening port");
DEFINE_bool(log_to_stderr, false, " log to stderr?");
DEFINE_string(modelsPath, "models", " subj");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  FLAGS_logtostderr = FLAGS_log_to_stderr;

  google::InitGoogleLogging(argv[0]);


  std::string mode = argv[1];
  std::vector<std::string> modes = {"server", "languages", "news", "categories", "threads"};
  if (std::find(modes.begin(), modes.end(), mode) == modes.end()) {
    std::cerr << "unknown mode " << mode << std::endl;
    return -1;
  }

  std::cout << "Runnnig mode - " << mode << std::endl;

  tgnews::Context context(FLAGS_modelsPath);

  tgnews::FileManager file_manager;
  tgnews::FileCache file_cache(file_manager);

  if (mode == "server") {
    tgnews::Server server(FLAGS_port);
    server.Run();
  } else if (mode == "languages") {
    
  } else if (mode == "news") {

  } else if (mode == "categories") {

  } else if (mode == "threads") {

  }
  return 0;
}

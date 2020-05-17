#include "server/server.h"
#include "server/file_cache.h"
#include "server/context.h"

#include "solver/response_builder.h"

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
  std::string content_dir = argv[2];
  std::vector<std::string> modes = {"server", "languages", "news", "categories", "threads"};
  if (std::find(modes.begin(), modes.end(), mode) == modes.end()) {
    LOG(FATAL) << fmt::format("unknown mode: {}", mode);
  }

  LOG(INFO) << fmt::format("Running mode - {}", mode);

  tgnews::Context context(FLAGS_modelsPath);
  tgnews::ResponseBuilder responseBuilder(std::move(context));

  auto file_manager = std::make_unique<tgnews::FileManager>(std::move(content_dir));
  tgnews::FileCache file_cache(file_manager.get());

  if (mode == "server") {
    tgnews::Server server(FLAGS_port, std::move(file_manager));
    server.Run();
  } else if (mode == "languages") {
    if (file_cache.GetDocuments().size() == 0) {
      std::cerr << "empty docs" << std::endl;
    }
    std::cout << responseBuilder.AddDocuments(file_cache.GetDocuments()).GetAns();
  } else if (mode == "news") {

  } else if (mode == "categories") {

  } else if (mode == "threads") {

  }
  return 0;
}

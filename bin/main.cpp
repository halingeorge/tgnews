#include "server/server.h"
#include "server/file_cache.h"

#include "base/context.h"
#include "base/util.h"

#include "solver/response_builder.h"

#include "gflags/gflags.h"
#include "glog/logging.h"

#include <iostream>

DEFINE_bool(log_to_stderr, false, " log to stderr?");
DEFINE_string(modelsPath, "models", " subj");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  FLAGS_logtostderr = FLAGS_log_to_stderr;

  google::InitGoogleLogging(argv[0]);

  std::string mode = argv[1];
  std::vector<std::string> modes = {"server", "languages", "news", "categories", "threads"};
  if (std::find(modes.begin(), modes.end(), mode) == modes.end()) {
    LOG(FATAL) << fmt::format("unknown mode: {}", mode);
  }

  LOG(INFO) << fmt::format("Running mode - {}", mode);

  tgnews::Context context(FLAGS_modelsPath);
  tgnews::ResponseBuilder responseBuilder(std::move(context));

  if (mode == "server") {
    int port = std::stoi(argv[2]);
    auto file_manager = std::make_unique<tgnews::FileManager>();
    tgnews::Server server(port, std::move(file_manager));
    server.Run();
    return 0;
  }

  std::string content_dir = argv[2];
  auto docs = tgnews::MakeDocumentsFromDir(content_dir);
  LOG(INFO) << fmt::format("Docs size- {}", docs.size());
  if (mode == "languages") {
    std::cout << responseBuilder.AddDocuments(docs).LangAns;
  } else if (mode == "news") {
    std::cout << responseBuilder.AddDocuments(docs).NewsAns;    
  } else if (mode == "categories") {
    std::cout << responseBuilder.AddDocuments(docs).CategoryAns;
  } else if (mode == "threads") {

  }
  return 0;
}

#include "base/file_cache.h"
#include "server/server.h"

#include "base/context.h"
#include "base/util.h"

#include "solver/response_builder.h"

#include "gflags/gflags.h"
#include "glog/logging.h"

#include <iostream>

DEFINE_string(modelsPath, "models", " subj");
DEFINE_int32(docsCount, -1, "how much docs to read, -1 to read all");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = true;

  google::InitGoogleLogging(argv[0]);

  std::string mode = argv[1];
  std::vector<std::string> modes = {"server", "languages", "news", "categories", "threads"};
  if (std::find(modes.begin(), modes.end(), mode) == modes.end()) {
    LOG(FATAL) << fmt::format("unknown mode: {}", mode);
  }

  LOG(INFO) << fmt::format("Running mode - {}", mode);

  tgnews::Context context(FLAGS_modelsPath, nullptr);

  LOG(INFO) << "context loaded";

  tgnews::ResponseBuilder responseBuilder(&context);

  LOG(INFO) << "response builder created";

  if (mode == "server") {
    int port = std::stoi(argv[2]);
    LOG(INFO) << fmt::format("prepare to run on port: {}", port);
    std::experimental::thread_pool pool(4);
    auto file_manager = std::make_unique<tgnews::FileManager>(pool, &context);
    tgnews::Server server(port, std::move(file_manager), pool, &responseBuilder);
    server.Run();
    return 0;
  }

  std::string content_dir = argv[2];
  auto docs = tgnews::MakeDocumentsFromDir(content_dir, FLAGS_docsCount, &context);
  LOG(INFO) << fmt::format("Docs size - {}", docs.size());
  if (mode == "languages") {
    std::cout << responseBuilder.AddDocuments(docs).LangAns.dump(4);
  } else if (mode == "news") {
    std::cout << responseBuilder.AddDocuments(docs).NewsAns.dump(4);
  } else if (mode == "categories") {
    std::cout << responseBuilder.AddDocuments(docs).CategoryAns.dump(4);
  } else if (mode == "threads") {
    std::cout << responseBuilder.AddDocuments(docs).ThreadsAns.dump(4);
  }
  return 0;
}

#include "base/document.h"
#include "base/util.h"
#include "base/time_helpers.h"

#include "test/common/common.h"

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "fmt/format.h"

#include "third_party/simple_web_server/client_http.hpp"

#include <chrono>
#include <vector>
#include <thread>

DEFINE_string(server_hostname, "localhost", "server hostname");
DEFINE_int32(server_port, 12345, "server port");
DEFINE_int32(thread_count, 2, "thread count for shooting");
DEFINE_int32(shooting_time, 60, "time to shoot in seconds");
DEFINE_bool(log_to_stderr, false, "log to stderr");
DEFINE_string(content_path, "content", "content path");

using namespace tgnews;

namespace {

std::vector<TestDocument> GetWorkerDocuments(std::vector<TestDocument>& documents,
                                                 size_t worker_id,
                                                 size_t thread_count) {
  std::vector<TestDocument> worker_documents;
  for (size_t i = worker_id; i < documents.size(); i += thread_count) {
    worker_documents.push_back(std::move(documents[i]));
  }
  return worker_documents;
}

}  // namespace

class Worker {
 public:
  Worker(std::vector<TestDocument> documents)
      : deadline_(Deadline(std::chrono::seconds(FLAGS_shooting_time))), documents_(std::move(documents)),
        client_(std::make_unique<SimpleWeb::Client<SimpleWeb::HTTP>>(fmt::format("{}:{}",
                                                                                 FLAGS_server_hostname,
                                                                                 FLAGS_server_port))) {
  }

  void Run() {
    while (Now() < deadline_) {
      DoIteration();
    }
  }

 private:
  void DoIteration() {
    for (auto document : documents_) {
      PutRequest(*client_, document.name, document.content, document.max_age);
    }
    WaitForSubsetDocuments(*client_, GetDocumentNames(documents_));
    for (auto document : documents_) {
      PutRequest(*client_, document.name, document.content, document.max_age, "204 Created");
    }
    WaitForSubsetDocuments(*client_, GetDocumentNames(documents_));
    for (auto document : documents_) {
      DeleteRequest(*client_, document.name);
    }
    WaitForNoneDocuments(*client_, GetDocumentNames(documents_));
    for (auto document : documents_) {
      DeleteRequest(*client_, document.name, "404 Not Found");
    }
    WaitForNoneDocuments(*client_, GetDocumentNames(documents_));
  }

 private:
  std::chrono::system_clock::time_point deadline_;
  std::vector<TestDocument> documents_;
  std::unique_ptr<SimpleWeb::Client<SimpleWeb::HTTP>> client_;
};

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  FLAGS_logtostderr = FLAGS_log_to_stderr;

  google::InitGoogleLogging(argv[0]);

  std::vector<TestDocument> documents;
  for (auto document : MakeDocumentsFromDir(FLAGS_content_path)) {
    documents.push_back(TestDocument(document->name, document->content, std::chrono::seconds(60)));
  }
  LOG(INFO) << "document count: " << documents.size();

  std::vector<std::thread> workers;

  size_t thread_count = FLAGS_thread_count;
  for (size_t worker_id = 0; worker_id < thread_count; worker_id++) {
    auto work = [&] {
      Worker worker(GetWorkerDocuments(documents, worker_id, thread_count));
      worker.Run();
    };
    if (worker_id + 1 == thread_count) {
      work();
    } else {
      workers.emplace_back(std::move(work));
    }
  }

  for (auto& worker : workers) {
    worker.join();
  }
  return 0;
}

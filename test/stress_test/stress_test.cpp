#include <chrono>
#include <thread>
#include <vector>

#include "base/document.h"
#include "base/time_helpers.h"
#include "base/util.h"
#include "fmt/format.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "test/common/common.h"
#include "third_party/simple_web_server/client_http.hpp"

DEFINE_string(server_hostname, "localhost", "server hostname");
DEFINE_int32(server_port, 12345, "server port");
DEFINE_int32(thread_count, 2, "thread count for shooting");
DEFINE_int32(shooting_time, 60, "time to shoot in seconds");
DEFINE_bool(log_to_stderr, false, "log to stderr");
DEFINE_string(content_path, "content", "content path");

using namespace tgnews;

namespace {

std::vector<TestDocument> GetWorkerDocuments(
    std::vector<TestDocument>& documents, size_t worker_id,
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
  Worker(std::vector<TestDocument> documents, Stats& stats)
      : deadline_(Deadline(std::chrono::seconds(FLAGS_shooting_time))),
        documents_(std::move(documents)),
        client_(std::make_unique<SimpleWeb::Client<SimpleWeb::HTTP>>(
            fmt::format("{}:{}", FLAGS_server_hostname, FLAGS_server_port))),
        stats_(stats) {}

  void Run() {
    while (Now() < deadline_) {
      DoIteration();
    }
  }

 private:
  void DoIteration() {
    for (auto document : documents_) {
      MakeRequest([&] {
        PutRequest(*client_, document.name, document.content, document.max_age);
      });
    }
    MakeRequest([&] {
      GetArticles(*client_);
    });
    WaitForSubsetDocuments(*client_, GetDocumentNames(documents_));
    for (auto document : documents_) {
      MakeRequest([&] {
        PutRequest(*client_, document.name, document.content, document.max_age,
                   "204 Created");
      });
    }
    MakeRequest([&] {
      GetArticles(*client_);
    });
    WaitForSubsetDocuments(*client_, GetDocumentNames(documents_));
    for (auto document : documents_) {
      MakeRequest([&] {
        DeleteRequest(*client_, document.name);
      });
    }
    MakeRequest([&] {
      GetArticles(*client_);
    });
    WaitForSubsetDocuments(*client_, GetDocumentNames(documents_));
    for (auto document : documents_) {
      MakeRequest([&] {
        DeleteRequest(*client_, document.name, "404 No Content");
      });
    }
    MakeRequest([&] {
      GetArticles(*client_);
    });
    WaitForSubsetDocuments(*client_, GetDocumentNames(documents_));
  }

  template <typename F>
  void MakeRequest(F&& f) {
    Stats::State state;
    stats_.OnStart();
    f();
    stats_.OnFinish(state);
  }

 private:
  std::chrono::system_clock::time_point deadline_;
  std::vector<TestDocument> documents_;
  std::unique_ptr<SimpleWeb::Client<SimpleWeb::HTTP>> client_;
  Stats& stats_;
};

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  FLAGS_minloglevel = 1;
  FLAGS_logtostderr = FLAGS_log_to_stderr;

  google::InitGoogleLogging(argv[0]);

  std::vector<TestDocument> documents;
  for (const auto& document : MakeDocumentsFromDir(FLAGS_content_path)) {
    documents.push_back(TestDocument(document.name, document.content,
                                     std::chrono::seconds(60)));
  }
  LOG(INFO) << "document count: " << documents.size();

  std::vector<std::thread> workers;

  Stats stats;

  size_t thread_count = FLAGS_thread_count;
  for (size_t worker_id = 0; worker_id < thread_count; worker_id++) {
    auto work = [&](auto id) {
      Worker worker(GetWorkerDocuments(documents, id, thread_count), stats);
      worker.Run();
    };
    if (worker_id + 1 == thread_count) {
      work(worker_id);
    } else {
      workers.emplace_back(std::move(work), worker_id);
    }
  }

  for (auto& worker : workers) {
    worker.join();
  }
  return 0;
}

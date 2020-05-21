#include <random>

#include "base/time_helpers.h"
#include "gtest/gtest.h"
#include "server/server.h"
#include "test/common/common.h"
#include "third_party/simple_web_server/client_http.hpp"

using namespace tgnews;
using namespace std::chrono_literals;
using namespace testing;

namespace {

constexpr size_t kPort = 12345;
constexpr const char* kContentDir = "test_content";

}  // namespace

class ServerTest : public Test {
 public:
  ServerTest() : mt(rd()) {
    FLAGS_minloglevel = 1;

    boost::filesystem::path content_path = kContentDir;
    if (boost::filesystem::exists(content_path)) {
      LOG_IF(FATAL, !boost::filesystem::remove_all(content_path))
          << "unable to remove content dir: " << kContentDir;
    }
    LOG_IF(FATAL, !boost::filesystem::create_directory(content_path))
        << "unable to create content dir: " << kContentDir;
    server = std::make_unique<Server>(
        kPort, std::make_unique<FileManager>(pool_, kContentDir), pool_);

    server_thread_ = std::make_unique<std::thread>([&]() { server->Run(); });

    std::this_thread::sleep_for(1s);

    client = std::make_unique<SimpleWeb::Client<SimpleWeb::HTTP>>(
        fmt::format("localhost:{0}", kPort));
  }

  ~ServerTest() {
    server->Stop();

    server_thread_->join();

    boost::filesystem::path content_path = kContentDir;
    LOG_IF(FATAL, !boost::filesystem::remove_all(content_path))
        << "unable to remove content dir: " << kContentDir;
    LOG_IF(FATAL, boost::filesystem::exists(content_path))
        << "path still exists: " << kContentDir;
  }

  std::unique_ptr<Server> server;
  std::random_device rd;
  std::mt19937 mt;

  std::unique_ptr<SimpleWeb::Client<SimpleWeb::HTTP>> client;

 private:
  std::unique_ptr<std::thread> server_thread_;
  std::experimental::thread_pool pool_{1};
};

TEST_F(ServerTest, TestDeleteNonexistent) {
  static constexpr size_t kAddDocuments = 10;
  static constexpr size_t kRemoveDocuments = 6;

  SimpleWeb::Client<SimpleWeb::HTTP> client(
      fmt::format("localhost:{0}", kPort));

  DeleteRequest(client, "nonexistent_document", "404 No Content");
}

TEST_F(ServerTest, TestPutAndRemoveDocuments) {
  static constexpr size_t kAddDocuments = 10;
  static constexpr size_t kRemoveDocuments = 6;

  auto documents = GenerateDocuments(mt, /*count =*/kAddDocuments);
  for (auto& document : documents) {
    PutRequest(*client, document.name, document.content, document.max_age);
  }

  EXPECT_TRUE(WaitForExactDocuments(*client, GetDocumentNames(documents)));

  std::shuffle(documents.begin(), documents.end(), mt);
  for (size_t i = 0; i < kRemoveDocuments; i++) {
    DeleteRequest(*client, documents[i].name);
  }

  EXPECT_TRUE(WaitForExactDocuments(
      *client, GetDocumentNames(
                   {documents.begin() + kRemoveDocuments, documents.end()})));
}

TEST_F(ServerTest, TestPutAndUpdateDocument) {
  static constexpr size_t kAddDocuments = 10;
  static constexpr size_t kRemoveDocuments = 6;

  auto documents = GenerateDocuments(mt, /*count =*/kAddDocuments);
  for (auto& document : documents) {
    PutRequest(*client, document.name, document.content, document.max_age);
  }

  EXPECT_TRUE(WaitForExactDocuments(*client, GetDocumentNames(documents)));

  std::shuffle(documents.begin(), documents.end(), mt);
  for (size_t i = 0; i < kRemoveDocuments; i++) {
    auto& document = documents[i];
    DeleteRequest(*client, document.name);
  }
  for (size_t i = kRemoveDocuments; i < documents.size(); i++) {
    auto& document = documents[i];
    PutRequest(*client, document.name, document.content, document.max_age,
               "204 Created");
  }

  EXPECT_TRUE(WaitForExactDocuments(
      *client, GetDocumentNames(
                   {documents.begin() + kRemoveDocuments, documents.end()})));
}

TEST_F(ServerTest, TestDocumentsDeadline) {
  static constexpr size_t k3SecondsCount = 10;
  static constexpr size_t k6SecondsCount = 8;
  static constexpr size_t k9SecondsCount = 9;

  auto documents3 = GenerateDocuments(mt, k3SecondsCount, 3s);
  auto documents6 = GenerateDocuments(mt, k3SecondsCount, 6s);
  auto documents9 = GenerateDocuments(mt, k3SecondsCount, 9s);

  for (auto& document : ConcatDocuments(documents3, documents6, documents9)) {
    PutRequest(*client, document.name, document.content, document.max_age);
  }

  EXPECT_TRUE(WaitForExactDocuments(
      *client,
      GetDocumentNames(ConcatDocuments(documents3, documents6, documents9))));
  EXPECT_TRUE(WaitForExactDocuments(
      *client, GetDocumentNames(ConcatDocuments(documents6, documents9))));
  EXPECT_TRUE(WaitForExactDocuments(
      *client, GetDocumentNames(ConcatDocuments(documents9))));
}

TEST_F(ServerTest, TestUpdateDeadline) {
  auto document = GenerateDocument(mt, 3s);
  PutRequest(*client, document.name, document.content, document.max_age);
  EXPECT_TRUE(WaitForExactDocuments(*client, {document.name}));
  document.max_age = 6s;
  PutRequest(*client, document.name, document.content, document.max_age,
             "204 Created");
  EXPECT_TRUE(!WaitForExactDocuments(*client, {}));
  EXPECT_TRUE(WaitForExactDocuments(*client, {}));
}

#include "server/server.h"

#include "third_party/simple_web_server/client_http.hpp"

#include <boost/lexical_cast.hpp>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <random>

using namespace tgnews;
using namespace std::chrono_literals;
using namespace testing;

namespace {

constexpr size_t kPort = 12345;
constexpr const char* kContentDir = "test_content";

void PutRequest(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                std::string_view filename,
                std::string_view content,
                uint64_t max_age) {
  LOG(INFO) << fmt::format("put filename {0} content {1} max_age {2}", filename, content, max_age);
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Content-Type", "text/html");
  headers.emplace("Cache-Control", fmt::format("max-age={0}", max_age));
  headers.emplace("Content-Length", fmt::format("{0}", content.size()));
  auto response = client.request("PUT", fmt::format("/{0}", filename), content, headers);
  EXPECT_EQ(response->status_code, "201 Created");
}

void DeleteRequest(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                   std::string_view filename,
                   std::string_view expected_status = "204 No Content") {
  LOG(INFO) << fmt::format("remove filename {0}", filename);
  auto response = client.request("DELETE", fmt::format("/{0}", filename));
  EXPECT_EQ(response->status_code, expected_status);
}

std::string GetHeaderValue(const SimpleWeb::CaseInsensitiveMultimap& headers, std::string_view header_key) {
  auto header_it = headers.find(header_key.data());
  VERIFY(header_it != headers.end(), fmt::format("no header {0} found", header_key));
  return header_it->second;
}

std::vector<std::string> GetArticles(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                                     uint64_t period = 60,
                                     std::string_view lang_code = "ru",
                                     std::string_view category = "cars") {
  auto response =
      client.request("GET",
                     fmt::format("/threads?period={0}&lang_code={1}&category={2}", period, lang_code, category),
          /*content =*/ "");

  std::vector<std::string> articles;
  EXPECT_EQ(response->status_code, "200 OK");
  EXPECT_EQ(GetHeaderValue(response->header, "Content-Type"), "application/json");
  EXPECT_GT(boost::lexical_cast<size_t>(GetHeaderValue(response->header, "Content-Length")), 0);

  Json::Value value;
  Json::Reader reader;
  VERIFY(reader.parse(response->content.string(), value),
         fmt::format("error while reading articles: {}", reader.getFormattedErrorMessages()))
  LOG(INFO) << "articles: " << value.toStyledString();
  for (const auto& article : value["articles"]) {
    articles.push_back(article.asString());
  }

  return articles;
}

std::chrono::system_clock::time_point Now() {
  return std::chrono::system_clock::now();
}

std::chrono::system_clock::time_point Deadline(std::chrono::seconds seconds) {
  return Now() + seconds;
}

std::string GenerateString(std::mt19937& mt, size_t length) {
  std::uniform_int_distribution<int> distribution{'a', 'z'};

  std::string random_string(length, 0);
  for (auto& ch : random_string) {
    ch = distribution(mt);
  }

  return random_string;
}

Document GenerateDocument(std::mt19937& mt, uint64_t deadline = std::numeric_limits<uint64_t>::max()) {
  std::string name = GenerateString(mt, 10);
  std::string content = GenerateString(mt, 10);
  return Document(std::move(name), std::move(content), deadline);
}

std::vector<Document> GenerateDocuments(std::mt19937& mt, size_t count) {
  std::vector<Document> documents;
  documents.reserve(count);
  for (size_t i = 0; i < count; i++) {
    documents.emplace_back(GenerateDocument(mt));
  }
  return documents;
}

bool WaitForExactDocuments(SimpleWeb::Client<SimpleWeb::HTTP>& client, std::vector<std::string> names) {
  auto deadline = Deadline(5s);
  while (Now() < deadline) {
    auto articles = GetArticles(client);
    if (articles.size() == names.size()) {
      EXPECT_THAT(articles, UnorderedElementsAreArray(names));
      return true;
    }
    std::this_thread::sleep_for(1s);
  }
  return false;
}

std::vector<std::string> GetDocumentNames(const std::vector<Document>& documents) {
  std::vector<std::string> document_names;
  std::transform(documents.begin(), documents.end(), std::back_inserter(document_names), [](const auto& document) {
    return document.name;
  });
  return document_names;
}

}  // namespace

class ServerTest : public Test {
 public:
  ServerTest() : mt(rd()) {
    boost::filesystem::path content_path = kContentDir;
    if (boost::filesystem::exists(content_path)) {
      LOG_IF(FATAL, !boost::filesystem::remove_all(content_path)) << "unable to remove content dir: " << kContentDir;
    }
    LOG_IF(FATAL, !boost::filesystem::create_directory(content_path)) << "unable to create content dir: " << kContentDir;
    server = std::make_unique<Server>(kPort, std::make_unique<FileManager>(kContentDir));

    server_thread_ = std::make_unique<std::thread>([&]() {
      server->Run();
    });

    std::this_thread::sleep_for(1s);
  }

  ~ServerTest() {
    server->Stop();

    server_thread_->join();

    boost::filesystem::path content_path = kContentDir;
    LOG_IF(FATAL, !boost::filesystem::remove_all(content_path)) << "unable to remove content dir: " << kContentDir;
    LOG_IF(FATAL, boost::filesystem::exists(content_path)) << "path still exists: " << kContentDir;
  }

  std::unique_ptr<Server> server;
  std::random_device rd;
  std::mt19937 mt;

 private:
  std::unique_ptr<std::thread> server_thread_;
};

TEST_F(ServerTest, TestDeleteNonexistent) {
  static constexpr size_t kAddDocuments = 10;
  static constexpr size_t kRemoveDocuments = 6;

  SimpleWeb::Client<SimpleWeb::HTTP> client(fmt::format("localhost:{0}", kPort));

  DeleteRequest(client, "nonexistent_document", "404 No Content");
}

TEST_F(ServerTest, TestPutAndRemoveDocuments) {
  static constexpr size_t kAddDocuments = 10;
  static constexpr size_t kRemoveDocuments = 6;

  SimpleWeb::Client<SimpleWeb::HTTP> client(fmt::format("localhost:{0}", kPort));

  auto documents = GenerateDocuments(mt, /*count =*/ kAddDocuments);
  for (auto& document : documents) {
    PutRequest(client, document.name, document.content, document.deadline);
  }

  EXPECT_TRUE(WaitForExactDocuments(client, GetDocumentNames(documents)));

  std::shuffle(documents.begin(), documents.end(), mt);
  for (size_t i = 0; i < kRemoveDocuments; i++) {
    DeleteRequest(client, documents[i].name);
  }

  EXPECT_TRUE(WaitForExactDocuments(client, GetDocumentNames({documents.begin() + kRemoveDocuments, documents.end()})));
}

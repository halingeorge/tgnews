#include "base/time_helpers.h"
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

struct Document {
  Document(std::string name, std::string content, std::chrono::seconds max_age)
      : name(std::move(name)), content(std::move(content)), max_age(max_age) {}

  std::string name;
  std::string content;
  std::chrono::seconds max_age;
};

void PutRequest(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                std::string_view filename,
                std::string_view content,
                std::chrono::seconds max_age,
                std::string_view expected_status = "201 Created") {
  LOG(INFO) << fmt::format("put filename {0} content {1} max_age {2}",
                           filename,
                           content,
                           max_age.count());
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Content-Type", "text/html");
  headers.emplace("Cache-Control", fmt::format("max-age={0}", max_age.count()));
  headers.emplace("Content-Length", fmt::format("{0}", content.size()));
  auto response = client.request("PUT", fmt::format("/{0}", filename), content, headers);
  EXPECT_EQ(response->status_code, expected_status);
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
                                     std::chrono::seconds period,
                                     std::string_view lang_code,
                                     std::string_view category) {
  auto response =
      client.request("GET",
                     fmt::format("/threads?period={0}&lang_code={1}&category={2}", period.count(), lang_code, category),
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

std::string GenerateString(std::mt19937& mt, size_t length) {
  std::uniform_int_distribution<int> distribution{'a', 'z'};

  std::string random_string(length, 0);
  for (auto& ch : random_string) {
    ch = distribution(mt);
  }

  return random_string;
}

Document GenerateDocument(std::mt19937& mt, std::chrono::seconds max_age = 1024s) {
  std::string name = GenerateString(mt, 10);
  std::string content = GenerateString(mt, 1000);
  return Document(std::move(name), std::move(content), max_age);
}

std::vector<Document> GenerateDocuments(std::mt19937& mt, size_t count, std::chrono::seconds max_age = 1024s) {
  std::vector<Document> documents;
  documents.reserve(count);
  for (size_t i = 0; i < count; i++) {
    documents.emplace_back(GenerateDocument(mt, max_age));
  }
  return documents;
}

std::vector<std::string> GetDocumentNames(const std::vector<Document>& documents) {
  std::vector<std::string> document_names;
  std::transform(documents.begin(), documents.end(), std::back_inserter(document_names), [](const auto& document) {
    return document.name;
  });
  return document_names;
}

template <typename T>
std::vector<T> ConcatDocuments(const std::vector<T>& lhs) {
  return lhs;
}

template<typename T, typename... Args>
std::vector<T> ConcatDocuments(const std::vector<T>& lhs, Args&& ... args) {
  auto result = ConcatDocuments(std::forward<Args>(args)...);
  result.insert(result.end(), lhs.begin(), lhs.end());
  return result;
}

std::vector<std::string> GetArticles(SimpleWeb::Client<SimpleWeb::HTTP>& client) {
  auto ru_articles = GetArticles(client, 111111111s, "ru", "any");
  auto en_articles = GetArticles(client, 111111111s, "en", "any");

  auto articles = ConcatDocuments(ru_articles, en_articles);

  std::sort(articles.begin(), articles.end());
  articles.resize(std::unique(articles.begin(), articles.end()) - articles.begin());
  
  return articles;
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

}  // namespace

class ServerTest : public Test {
 public:
  ServerTest() : mt(rd()) {
    FLAGS_minloglevel = 1;

    boost::filesystem::path content_path = kContentDir;
    if (boost::filesystem::exists(content_path)) {
      LOG_IF(FATAL, !boost::filesystem::remove_all(content_path)) << "unable to remove content dir: " << kContentDir;
    }
    LOG_IF(FATAL, !boost::filesystem::create_directory(content_path))
            << "unable to create content dir: " << kContentDir;
    server = std::make_unique<Server>(kPort, std::make_unique<FileManager>(kContentDir));

    server_thread_ = std::make_unique<std::thread>([&]() {
      server->Run();
    });

    std::this_thread::sleep_for(1s);

    client = std::make_unique<SimpleWeb::Client<SimpleWeb::HTTP>>(fmt::format("localhost:{0}", kPort));
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

  std::unique_ptr<SimpleWeb::Client<SimpleWeb::HTTP>> client;

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

  auto documents = GenerateDocuments(mt, /*count =*/ kAddDocuments);
  for (auto& document : documents) {
    PutRequest(*client, document.name, document.content, document.max_age);
  }

  EXPECT_TRUE(WaitForExactDocuments(*client, GetDocumentNames(documents)));

  std::shuffle(documents.begin(), documents.end(), mt);
  for (size_t i = 0; i < kRemoveDocuments; i++) {
    DeleteRequest(*client, documents[i].name);
  }

  EXPECT_TRUE(WaitForExactDocuments(*client,
                                    GetDocumentNames({documents.begin() + kRemoveDocuments, documents.end()})));
}

TEST_F(ServerTest, TestPutAndUpdateDocument) {
  static constexpr size_t kAddDocuments = 10;
  static constexpr size_t kRemoveDocuments = 6;

  auto documents = GenerateDocuments(mt, /*count =*/ kAddDocuments);
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
    PutRequest(*client, document.name, document.content, document.max_age, "204 Created");
  }

  EXPECT_TRUE(WaitForExactDocuments(*client,
                                    GetDocumentNames({documents.begin() + kRemoveDocuments, documents.end()})));
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

  EXPECT_TRUE(WaitForExactDocuments(*client, GetDocumentNames(ConcatDocuments(documents3, documents6, documents9))));
  EXPECT_TRUE(WaitForExactDocuments(*client, GetDocumentNames(ConcatDocuments(documents6, documents9))));
  EXPECT_TRUE(WaitForExactDocuments(*client, GetDocumentNames(ConcatDocuments(documents9))));
}

TEST_F(ServerTest, TestUpdateDeadline) {
  auto document = GenerateDocument(mt, 3s);
  PutRequest(*client, document.name, document.content, document.max_age);
  EXPECT_TRUE(WaitForExactDocuments(*client, {document.name}));
  document.max_age = 6s;
  PutRequest(*client, document.name, document.content, document.max_age, "204 Created");
  EXPECT_TRUE(!WaitForExactDocuments(*client, {}));
  EXPECT_TRUE(WaitForExactDocuments(*client, {}));
}

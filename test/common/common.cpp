#include "common.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "test/common/constants.h"

namespace tgnews {

using namespace tgnews;
using namespace testing;

void PutRequest(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                std::string_view filename, std::string_view content,
                std::chrono::seconds max_age,
                std::string_view expected_status) {
  LOG(INFO) << fmt::format("put filename {0} content {1} max_age {2}", filename,
                           content, max_age.count());
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Content-Type", "text/html");
  headers.emplace("Cache-Control", fmt::format("max-age={0}", max_age.count()));
  headers.emplace("Content-Length", fmt::format("{0}", content.size()));
  auto response =
      client.request("PUT", fmt::format("/{0}", filename), content, headers);
  if (expected_status.empty()) {
    return;
  }
  EXPECT_EQ(response->status_code, expected_status);
}

void DeleteRequest(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                   std::string_view filename,
                   std::string_view expected_status) {
  LOG(INFO) << fmt::format("remove filename {0}", filename);
  auto response = client.request("DELETE", fmt::format("/{0}", filename));
  if (expected_status.empty()) {
    return;
  }
  EXPECT_EQ(response->status_code, expected_status);
}

std::string GetHeaderValue(const SimpleWeb::CaseInsensitiveMultimap& headers,
                           std::string_view header_key) {
  auto header_it = headers.find(header_key.data());
  VERIFY(header_it != headers.end(),
         fmt::format("no header {0} found", header_key));
  return header_it->second;
}

std::vector<std::string> GetArticles(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                                     std::chrono::seconds period,
                                     std::string_view lang_code,
                                     std::string_view category) {
  auto response = client.request(
      "GET",
      fmt::format("/threads?period={0}&lang_code={1}&category={2}",
                  period.count(), lang_code, category),
      /*content =*/"");

  EXPECT_EQ(response->status_code, "200 OK");
  EXPECT_EQ(GetHeaderValue(response->header, "Content-Type"),
            "application/json");
  EXPECT_GT(boost::lexical_cast<size_t>(
                GetHeaderValue(response->header, "Content-Length")),
            0);

  nlohmann::json value = nlohmann::json::parse(response->content.string());

  LOG(INFO) << "articles: " << value.dump(4);
  std::vector<std::string> articles;
  for (const auto& article : value["articles"]) {
    articles.push_back(article.get<std::string>());
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

TestDocument GenerateDocument(std::mt19937& mt, std::chrono::seconds max_age) {
  std::string name = GenerateString(mt, 10);
  std::string content = MakeXmlDocument();
  return TestDocument(std::move(name), std::move(content), max_age);
}

std::vector<TestDocument> GenerateDocuments(std::mt19937& mt, size_t count,
                                            std::chrono::seconds max_age) {
  std::vector<TestDocument> documents;
  documents.reserve(count);
  for (size_t i = 0; i < count; i++) {
    documents.emplace_back(GenerateDocument(mt, max_age));
  }
  return documents;
}

std::vector<std::string> GetDocumentNames(
    const std::vector<TestDocument>& documents) {
  std::vector<std::string> document_names;
  std::transform(documents.begin(), documents.end(),
                 std::back_inserter(document_names),
                 [](const auto& document) { return document.name; });
  return document_names;
}

std::vector<std::string> GetArticles(
    SimpleWeb::Client<SimpleWeb::HTTP>& client, bool update_time) {
  if (update_time) {
    std::stringstream thread_id;
    thread_id << std::this_thread::get_id();
    auto timer_updater_name =
        fmt::format("timer_updater_{}.html", thread_id.str());

    PutRequest(client, timer_updater_name, MakeXmlDocument(),
               std::chrono::seconds(60));
    DeleteRequest(client, timer_updater_name);
  }

  auto response = client.request("GET", "/_all_documents", /*content =*/"");

  EXPECT_EQ(response->status_code, "200 OK");
  EXPECT_EQ(GetHeaderValue(response->header, "Content-Type"),
            "application/json");
  EXPECT_GT(boost::lexical_cast<size_t>(
                GetHeaderValue(response->header, "Content-Length")),
            0);

  nlohmann::json value = nlohmann::json::parse(response->content.string());

  LOG(INFO) << "articles: " << value.dump(4);
  std::vector<std::string> articles;
  for (const auto& article : value["articles"]) {
    articles.push_back(article.get<std::string>());
  }
  return articles;
}

bool WaitForExactDocuments(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                           std::vector<std::string> names) {
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

bool WaitForSubsetDocuments(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                            std::vector<std::string> names) {
  auto deadline = Deadline(5s);
  while (Now() < deadline) {
    auto articles = GetArticles(client);
    bool success = true;
    for (const auto& name : names) {
      auto it = std::lower_bound(articles.begin(), articles.end(), name);
      if (it == articles.end()) {
        success = false;
        break;
      }
    }
    if (success) {
      return true;
    }
    std::this_thread::sleep_for(1s);
  }
  return false;
}

bool WaitForNoneDocuments(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                          std::vector<std::string> names) {
  auto deadline = Deadline(5s);
  while (Now() < deadline) {
    auto articles = GetArticles(client);
    bool success = true;
    for (const auto& name : names) {
      auto it = std::lower_bound(articles.begin(), articles.end(), name);
      if (it != articles.end()) {
        success = false;
        break;
      }
    }
    if (success) {
      return true;
    }
    std::this_thread::sleep_for(1s);
  }
  return false;
}

}  // namespace tgnews

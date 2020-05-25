#pragma once

#include <boost/lexical_cast.hpp>
#include <random>

#include "base/time_helpers.h"
#include "server/server.h"
#include "third_party/simple_web_server/client_http.hpp"

namespace tgnews {

using namespace std::chrono_literals;

struct TestDocument {
  TestDocument(std::string name, std::string content,
               std::chrono::seconds max_age)
      : name(std::move(name)), content(std::move(content)), max_age(max_age) {}

  std::string name;
  std::string content;
  std::chrono::seconds max_age;
};

void PutRequest(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                std::string_view filename, std::string_view content,
                std::chrono::seconds max_age,
                std::string_view expected_status = "201 Created");

void DeleteRequest(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                   std::string_view filename,
                   std::string_view expected_status = "204 No Content");

std::string GetHeaderValue(const SimpleWeb::CaseInsensitiveMultimap& headers,
                           std::string_view header_key);

std::vector<std::string> GetArticles(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                                     std::chrono::seconds period,
                                     std::string_view lang_code,
                                     std::string_view category);

std::string GenerateString(std::mt19937& mt, size_t length);

TestDocument GenerateDocument(std::mt19937& mt,
                              std::chrono::seconds max_age = 1024s);

std::vector<TestDocument> GenerateDocuments(
    std::mt19937& mt, size_t count, std::chrono::seconds max_age = 1024s);

std::vector<std::string> GetDocumentNames(
    const std::vector<TestDocument>& documents);

template <typename T>
std::vector<T> ConcatDocuments(const std::vector<T>& lhs) {
  return lhs;
}

template <typename T, typename... Args>
std::vector<T> ConcatDocuments(const std::vector<T>& lhs, Args&&... args) {
  auto result = ConcatDocuments(std::forward<Args>(args)...);
  result.insert(result.end(), lhs.begin(), lhs.end());
  return result;
}

std::vector<std::string> GetArticles(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                                     bool update_time = true);

bool WaitForExactDocuments(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                           std::vector<std::string> names);

bool WaitForSubsetDocuments(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                            std::vector<std::string> names);

bool WaitForNoneDocuments(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                          std::vector<std::string> names);

}  // namespace tgnews

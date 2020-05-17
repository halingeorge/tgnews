#include "server/server.h"

#include "third_party/simple_web_server/client_http.hpp"

#include <boost/lexical_cast.hpp>

#include "gtest/gtest.h"

using namespace tgnews;

namespace {

void PutRequest(SimpleWeb::Client<SimpleWeb::HTTP>& client,
                std::string_view filename,
                std::string_view content,
                uint64_t max_age) {
  LOG(INFO) << fmt::format("put filename {0} content {1} max_age {2}", filename, content, max_age);
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Connection", "Keep-Alive");
  headers.emplace("Content-Type", "text/html");
  headers.emplace("Cache-Control", fmt::format("max-age={0}", max_age));
  headers.emplace("Content-Length", fmt::format("{0}", content.size()));
  auto response = client.request("PUT", fmt::format("/{0}", filename), content, headers);
  EXPECT_EQ(response->status_code, "201 Created");
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
  SimpleWeb::CaseInsensitiveMultimap headers;
  headers.emplace("Connection", "Keep-Alive");
  auto response =
      client.request("GET",
                     fmt::format("/threads?period={0}&lang_code={1}&category={2}", period, lang_code, category),
          /*content =*/ "", headers);

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

}  // namespace

TEST(ServerTest, LoadTest) {
  static constexpr size_t kPort = 12345;

  Server server(kPort, std::make_unique<FileManager>());

  std::thread server_thread([&server]() {
    server.Run();
    LOG(INFO) << "server stopped";
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));

  SimpleWeb::Client<SimpleWeb::HTTP> client(fmt::format("localhost:{0}", kPort));

  static constexpr char* kFilename = "document.html";
  static constexpr char* kContent = R"("{
      "a": "b",
      "c": "d"
  })";

  LOG(INFO) << "put request";
  PutRequest(client, kFilename, kContent, 10);

  std::this_thread::sleep_for(std::chrono::seconds(5));
  LOG(INFO) << "get articles";
  auto articles = GetArticles(client);
  EXPECT_EQ(articles.size(), 1);
  EXPECT_EQ(articles[0], kFilename);

  server.Stop();
  server_thread.join();
}

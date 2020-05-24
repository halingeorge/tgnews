#include "server/server.h"

#include <boost/lexical_cast.hpp>

#include "base/base.h"
#include "glog/logging.h"

namespace tgnews {

namespace {

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

void ParseResult(std::string_view data, std::string_view& result) {
  result = data;
}

template<typename T>
void ParseResult(std::string_view data, T& result) {
  LOG(INFO) << "parse string to T: " << data;
  result = boost::lexical_cast<T>(data);
}

template<typename T>
T GetHeaderValue(const SimpleWeb::CaseInsensitiveMultimap& headers,
                 std::string_view header_key) {
  LOG(INFO) << "get header value for key: " << header_key;
  auto header_it = headers.find(header_key.data());
  VERIFY(header_it != headers.end(),
         fmt::format("no header {0} found", header_key));
  T result;
  ParseResult(header_it->second, result);
  return result;
}

std::tuple<uint64_t, std::string, std::string> ParseThreadsRequest(
    std::string query_string) {
  LOG(INFO) << "parse threads request: " << query_string;
  const std::regex base_regex("period=([0-9]+)&lang_code=(.+)&category=(.+)");
  std::smatch base_match;
  VERIFY(std::regex_match(query_string, base_match, base_regex),
         "regex not matched");
  VERIFY(base_match.size() == 4,
         fmt::format("expected 4 args but got {0}", base_match.size()));

  auto period = boost::lexical_cast<uint64_t>(base_match[1].str());
  auto lang_code = base_match[2].str();
  auto category = base_match[3].str();

  return std::make_tuple(period, std::move(lang_code), std::move(category));
}

struct StatsHandler {
  StatsHandler(Stats& stats) : stats(stats) { stats.OnStart(); }

  void OnSuccess() {
    if (replies.fetch_add(1) == 0) {
      state.status = Stats::Status::Success;
      stats.OnFinish(state);
    }
  }

  void OnFailure() {
    if (replies.fetch_add(1) == 0) {
      state.status = Stats::Status::Failure;
      stats.OnFinish(state);
    }
  }

  Stats& stats;
  Stats::State state = {};
  std::atomic<size_t> replies = 0;
};

auto OnFailCallback(std::shared_ptr<HttpServer::Response> response,
                    std::shared_ptr<StatsHandler> stats_handler) {
  return [r = std::move(response), h = std::move(stats_handler)](std::exception_ptr ptr) {
    if (ptr) {
      try {
        std::rethrow_exception(ptr);
      } catch (std::exception& e) {
        LOG(ERROR) << "exception caught: " << e.what();
        r->write(
            SimpleWeb::StatusCode::server_error_internal_server_error,
            e.what());
        h->OnFailure();
      }
    }
  };
}

}  // namespace

Server::Server(uint32_t port, std::unique_ptr<FileManager> file_manager,
               std::experimental::thread_pool& pool, ResponseBuilder* response_builder)
    : port_(port),
      file_manager_(std::move(file_manager)),
      responses_cache_strand_(pool.get_executor()),
      response_builder_(response_builder),
      pool_(pool) {
  server_.config.port = port;

  SetupHandlers();
}

Server::~Server() { Stop(); }

void Server::Run() {
  LOG(INFO) << "starting server on port: " << port_;
  server_.start();
}

void Server::Stop() {
  server_.stop();
  pool_.stop();
  pool_.join();
  LOG(INFO) << "server stopped";
}

void Server::SetupHandlers() {
  server_.resource["^/(.+)$"]["PUT"] =
      [this](std::shared_ptr<HttpServer::Response> response,
             std::shared_ptr<HttpServer::Request> request) {
        auto stats_handler = std::make_shared<StatsHandler>(stats_);

        cti::make_continuable<void>([=](auto&& promise) mutable {
          file_manager_->RemoveOutdatedFiles();

          auto filename = request->path.substr(1);
          auto content = request->content.string();
          LOG(INFO) << "received put request: " << filename;
          LOG(INFO) << "file content: " << content;

          auto content_type =
              GetHeaderValue<std::string_view>(request->header, "Content-Type");
          VERIFY(content_type == "text/html",
                 fmt::format("unexpected content type: {0}", content_type));

          auto cache_control_str = GetHeaderValue<std::string_view>(
              request->header, "Cache-Control");
          uint64_t max_age;
          ParseResult(cache_control_str.substr(8), max_age);

          file_manager_->StoreOrUpdateFile(
                  std::move(filename), std::move(content), max_age).then([=](bool updated) {
                uint32_t status_code = updated ? 204 : 201;
                std::string data =
                    fmt::format("HTTP/1.1 {0} Created\r\n\r\n", status_code);
                response->write(data.data(), data.size());
                stats_handler->OnSuccess();

                UpdateResponseCache();
              })
              .fail(OnFailCallback(response, stats_handler));

          promise.set_value();
        }).fail(OnFailCallback(response, stats_handler));
      };

  server_.resource["^/(.+)$"]["DELETE"] =
      [this](std::shared_ptr<HttpServer::Response> response,
             std::shared_ptr<HttpServer::Request> request) {
        auto stats_handler = std::make_shared<StatsHandler>(stats_);

        cti::make_continuable<void>([=](auto&& promise) mutable {
          file_manager_->RemoveOutdatedFiles();

          auto filename = request->path.substr(1);
          LOG(INFO) << "received delete request: " << filename;
          file_manager_->RemoveFile(filename).then([=](bool removed) {
            uint32_t status_code = removed ? 204 : 404;
            std::string data =
                fmt::format("HTTP/1.1 {0} No Content\r\n\r\n", status_code);
            response->write(data.data(), data.size());
            stats_handler->OnSuccess();

            UpdateResponseCache();
          }).fail(OnFailCallback(response, stats_handler));

          promise.set_value();
        }).fail(OnFailCallback(response, stats_handler));
      };

  server_.resource["^/threads$"]["GET"] =
      [this](std::shared_ptr<HttpServer::Response> response,
             std::shared_ptr<HttpServer::Request> request) {
        auto stats_handler = std::make_shared<StatsHandler>(stats_);

        cti::make_continuable<void>([=](auto&& promise) mutable {
          file_manager_->RemoveOutdatedFiles();

          auto[period, lang_code, category] =
          ParseThreadsRequest(std::move(request->query_string));
          LOG(INFO) << fmt::format(
              "get threads with period={0} lang_code={1} category={2}", period,
              lang_code, category);
          GetDocumentThreads(period, lang_code, category)
              .then([=](auto&& value) {
                SimpleWeb::CaseInsensitiveMultimap headers;
                headers.emplace("Content-type", "application/json");
                response->write(value.dump(), headers);
                stats_handler->OnSuccess();
              }).fail(OnFailCallback(response, stats_handler));

          promise.set_value();
        }).fail(OnFailCallback(response, stats_handler));
      };

  server_.default_resource["GET"] =
      [this](std::shared_ptr<HttpServer::Response> response,
             std::shared_ptr<HttpServer::Request> request) {
        LOG(ERROR) << "unhandled request: " << request->path;
        response->write(SimpleWeb::StatusCode::server_error_not_implemented);
      };
}

cti::continuable<nlohmann::json> Server::GetDocumentThreads(
    uint64_t /*period*/, std::string /*lang_code*/, std::string /*category*/) {
  return file_manager_->GetDocuments().then(
      [](std::vector<DocumentConstPtr> documents) {
        nlohmann::json value;
        nlohmann::json articles = nlohmann::json::array();
        for (auto document_ptr : documents) {
          articles.push_back(document_ptr->name);
        }
        value["articles"] = std::move(articles);
        LOG(INFO) << "GetDocumentThreads: " << value;
        return value;
      });
}

void Server::UpdateResponseCache() {
  if (!response_builder_) {
    return;
  }
  file_manager_->FetchChangeLog().then([=](std::vector<Document> change_log) {
    if (change_log.empty()) {
      return;
    }
    std::experimental::dispatch(
        responses_cache_strand_, [this, change_log = std::move(change_log)]() mutable {
          auto response_cache =
              std::make_unique<CalculatedResponses>(response_builder_->AddDocuments(change_log));
          std::unique_lock lock(responses_cache_mutex_);
          responses_cache_ = std::move(response_cache);
        });
  });
}

}  // namespace tgnews

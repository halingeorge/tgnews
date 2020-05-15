#include "server/base.h"
#include "server/server.h"

#include "glog/logging.h"

#include <boost/lexical_cast.hpp>

namespace tgnews {

namespace {

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

void ParseResult(std::string_view data, std::string_view& result) {
  result = data;
}

template<typename T>
void ParseResult(std::string_view data, T& result) {
  result = boost::lexical_cast<T>(data);
}

template<typename T>
T GetHeaderValue(const SimpleWeb::CaseInsensitiveMultimap& headers, std::string_view header_key) {
  auto header_it = headers.find(header_key.data());
  VERIFY(header_it != headers.end(), fmt::format("no header {0} found", header_key));
  T result;
  ParseResult(header_it->second, result);
  return result;
}

}  // namespace

Server::Server(uint32_t port) : port_(port) {
  server_.config.port = port;

  SetupHandlers();
}

Server::~Server() { server_.stop(); }

void Server::Run() {
  LOG(INFO) << "starting server on port: " << port_;
  server_.start();
}

void Server::SetupHandlers() {
  server_.resource["^/(.+)$"]["PUT"] =
      [this](std::shared_ptr<HttpServer::Response> response,
             std::shared_ptr<HttpServer::Request> request) {
        auto filename = request->path.substr(1);
        auto content = request->content.string();
        LOG(INFO) << "received put request: " << filename;
        LOG(INFO) << "file content: " << content;

        auto content_type = GetHeaderValue<std::string_view>(request->header, "Content-Type");
        VERIFY(content_type == "text/html", fmt::format("unexpected content type: {0}", content_type));

        auto updated = file_manager_.StoreOrUpdateFile(std::move(filename),
                                                       std::move(content),
                                                       GetHeaderValue<uint64_t>(request->header, "Cache-Control"));

        uint32_t status_code = updated ? 204 : 201;
        std::string data = fmt::format("HTTP/1.1 {0} Created\r\n\r\n", status_code);
        response->write(data.data(), data.size());
      };

  server_.resource["^/(.+)$"]["DELETE"] =
      [this](std::shared_ptr<HttpServer::Response> response,
             std::shared_ptr<HttpServer::Request> request) {
        auto filename = request->path.substr(1);
        LOG(INFO) << "received delete request: " << filename;
        auto removed = file_manager_.RemoveFile(filename);

        uint32_t status_code = removed ? 204 : 404;
        std::string data = fmt::format("HTTP/1.1 {0} No Content\r\n\r\n", status_code);
        response->write(data.data(), data.size());
      };
}

}  // namespace tgnews
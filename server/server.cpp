#include "server/server.h"

#include "glog/logging.h"

namespace tgnews {

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

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
        LOG(INFO) << "received request: " << filename;
        LOG(INFO) << "file content: " << content;
        file_manager_.StoreFile(std::move(filename), std::move(content));

        std::string data = "HTTP/1.1 201 Created\r\n\r\n";
        response->write(data.data(), data.size());
      };
}

}  // namespace tgnews
#include "server/server.h"

#include "third_party/http_server/server.h"

#include "glog/logging.h"

namespace tgnews {

Server::Server(uint32_t port) : port_(port) {
  LOG(INFO) << "Server listening on: " << port;
}

void Server::Run() {
  SimpleWeb::Server server;
}

}  // namespace tgnews
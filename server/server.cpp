#include "server/server.h"

#include "glog/logging.h"

#include <iostream>

namespace tgnews {

Server::Server(uint32_t port) : port_(port) {
  LOG(INFO) << "Server listening on: " << port;
}

}  // namespace tgnews
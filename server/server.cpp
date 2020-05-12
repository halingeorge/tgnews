#include "server/server.h"

#include <iostream>

namespace tgnews {

Server::Server(uint32_t port) : port_(port) {
  std::cout << "Serving on port: " << port << std::endl;
}

}  // namespace tgnews
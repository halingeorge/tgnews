#pragma once

#include <cstdint>

#include "server/file_manager.h"
#include "third_party/simple_web_server/server_http.hpp"

namespace tgnews {

class Server {
 public:
  Server(uint32_t port);

  ~Server();

  uint32_t Port() const { return port_; }

  void Run();

 private:
  void SetupHandlers();

 private:
  uint32_t port_;
  FileManager file_manager_;
  SimpleWeb::Server<SimpleWeb::HTTP> server_;
};

}  // namespace tgnews
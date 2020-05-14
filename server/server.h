#pragma once

#include "third_party/simple_web_server/server_http.hpp"

#include <cstdint>

namespace tgnews {

class Server {
 public:
  Server();

  void Run();

 private:
  SimpleWeb::Server<SimpleWeb::HTTP> server_;
};

}  // namespace tgnews
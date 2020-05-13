#pragma once

#include <cstdint>

namespace tgnews {

class Server {
 public:
  Server(uint32_t port);

  uint32_t Port() const { return port_; }

  void Run();

 private:
  uint32_t port_;
};

}  // namespace tgnews
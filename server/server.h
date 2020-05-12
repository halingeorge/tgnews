#pragma once

#include <cstdint>

namespace tgnews {

class Server {
 public:
  Server(uint32_t port);

  uint32_t Port() const { return port_; }

 private:
  uint32_t port_;
};

}  // namespace tgnews
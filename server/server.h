#pragma once

#include <cstdint>

#include "server/file_manager.h"
#include "server_http.hpp"

namespace tgnews {

class Server {
 public:
  Server(uint32_t port, std::unique_ptr<FileManager> file_manager);

  ~Server();

  uint32_t Port() const { return port_; }

  void Run();

  void Stop();

 private:
  void SetupHandlers();

  Json::Value GetDocumentThreads(uint64_t period, std::string lang_code, std::string category);

 private:
  uint32_t port_;
  std::unique_ptr<FileManager> file_manager_;
  SimpleWeb::Server<SimpleWeb::HTTP> server_;
};

}  // namespace tgnews

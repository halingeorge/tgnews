#pragma once

#include <atomic>
#include <continuable/continuable.hpp>
#include <cstdint>
#include <experimental/strand>
#include <memory>
#include <shared_mutex>

#include "base/file_manager.h"
#include "server/stats.h"
#include "server_http.hpp"
#include "solver/response_builder.h"

namespace tgnews {

class Server {
 public:
  Server(uint32_t port, std::unique_ptr<FileManager> file_manager,
         std::experimental::thread_pool& pool,
         ResponseBuilder* response_builder = nullptr);

  ~Server();

  uint32_t Port() const { return port_; }

  void Run();

  void Stop();

 private:
  void SetupHandlers();

  cti::continuable<nlohmann::json> GetAllDocuments();

  nlohmann::json GetDocumentThreads(uint64_t period, std::string lang_code,
                                    std::string category);

  void UpdateResponseCache();

 private:
  uint32_t port_;
  std::unique_ptr<FileManager> file_manager_;
  SimpleWeb::Server<SimpleWeb::HTTP> server_;
  Stats stats_;

  std::experimental::strand<std::experimental::thread_pool::executor_type>
      responses_cache_strand_;
  ResponseBuilder* const response_builder_;
  std::unique_ptr<CalculatedResponses> responses_cache_;
  std::shared_mutex responses_cache_mutex_;

  std::experimental::thread_pool& pool_;
};

}  // namespace tgnews

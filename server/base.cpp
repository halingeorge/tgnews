#include "server/base.h"

namespace tgnews {

void LogAndThrow(const std::string& error_message) {
  LOG(ERROR) << error_message;
  throw std::runtime_error(error_message);
}

}  // namespace tgnews
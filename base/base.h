#pragma once

#include "glog/logging.h"

#include <stdexcept>

namespace tgnews {

inline void LogAndThrow(const std::string& error_message) {
  LOG(ERROR) << error_message;
  throw std::runtime_error(error_message);
}

#define VERIFY(EXP, ERROR) \
  if (!(EXP)) {            \
    LogAndThrow(ERROR);    \
  }

}  // namespace tgnews

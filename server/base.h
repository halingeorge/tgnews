#pragma once

#include <stdexcept>

#include "glog/logging.h"

namespace tgnews {

void LogAndThrow(const std::string& error_message);

#define VERIFY(EXP, ERROR) \
  if (!(EXP)) {            \
    LogAndThrow(ERROR);    \
  }

}  // namespace tgnews
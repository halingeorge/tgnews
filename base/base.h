#pragma once

#include "glog/logging.h"

#include <stdexcept>

namespace tgnews {

#define VERIFY(EXP, MSG)           \
  if (!(EXP)) {                    \
    LOG(ERROR) << MSG;             \
    throw std::runtime_error(MSG); \
  }

}  // namespace tgnews

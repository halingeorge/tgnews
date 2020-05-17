#include "base/time_helpers.h"

namespace tgnews {

std::chrono::system_clock::time_point Now() {
  return std::chrono::system_clock::now();
}

uint64_t NowCount() {
  return Now().time_since_epoch().count();
}

std::chrono::system_clock::time_point Deadline(std::chrono::seconds seconds) {
  return Now() + seconds;
}

uint64_t DeadlineCount(std::chrono::seconds seconds) {
  return Deadline(seconds).time_since_epoch().count();
}

}  // namespace tgnews
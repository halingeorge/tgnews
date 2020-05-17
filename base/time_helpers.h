#pragma once

#include <chrono>

namespace tgnews {

std::chrono::system_clock::time_point Now();

uint64_t NowCount();

std::chrono::system_clock::time_point Deadline(std::chrono::seconds seconds);

uint64_t DeadlineCount(std::chrono::seconds seconds);

}  // namespace tgnews
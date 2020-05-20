#pragma once

#include <atomic>
#include <chrono>

#include "fmt/format.h"
#include "glog/logging.h"

namespace tgnews {

namespace details {

class EveryKthHandle {
 public:
  explicit EveryKthHandle(size_t k) : k_(k) {}

  template <typename Handler>
  void Step(Handler&& handler) {
    auto value = current_.fetch_add(1);
    if ((value & k_) == 0) {
      handler();
    }
  }

 private:
  size_t k_;
  std::atomic<size_t> current_ = 0;
};

}  // namespace details

class Stats {
  // Has to be power of 2 minus 1.
  static constexpr size_t kDumpStatRequests = 1023;

 public:
  enum class Status { Success = 0, Failure };

  struct State {
    Status status = Status::Success;
    std::chrono::system_clock::time_point start_time =
        std::chrono::system_clock::now();
  };

 public:
  void OnStart() { ++in_progress_; }

  void OnFinish(State& state) {
    switch (state.status) {
      case Status::Success:
        ++success_;
        break;
      case Status::Failure:
        ++failure_;
        break;
    }

    auto finish_time = std::chrono::system_clock::now();

    max_latency_ =
        std::max(max_latency_.load(),
                 std::chrono::duration_cast<std::chrono::microseconds>(
                     finish_time - state.start_time));

    limiter_.Step([this] {
      LOG(ERROR) << fmt::format(
          "success: {} failure: {} in_progress: {} max_latency: {}us", success_,
          failure_, in_progress_, max_latency_.load().count());

      max_latency_ = std::chrono::microseconds(0);
    });

    --in_progress_;
  }

 private:
  std::atomic<size_t> success_ = 0;
  std::atomic<size_t> failure_ = 0;
  std::atomic<size_t> in_progress_ = 0;
  std::atomic<std::chrono::microseconds> max_latency_ =
      std::chrono::microseconds(0);
  details::EveryKthHandle limiter_{kDumpStatRequests};
};

}  // namespace tgnews
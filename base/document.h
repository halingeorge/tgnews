#pragma once

#include <chrono>
#include <string>

#include <memory>

namespace tgnews {

class Document {
 public:
  enum class State {
    Added = 0,
    Removed,
    Changed
  };

 public:
  Document(std::string name, std::string content, uint64_t deadline = 0, State state = State::Added);

 public:
  std::string name;
  std::string content;
  uint64_t deadline;
  State state;
};

using DocumentConstPtr = std::shared_ptr<const Document>;

}

#pragma once

#include <chrono>
#include <string>

namespace tgnews {

class Document {
 public:
  enum class State {
    Added = 0,
    Removed,
    Changed
  };

 public:
  Document(std::string name, std::string content, uint64_t deadline, State state);

 public:
  std::string name;
  std::string content;
  uint64_t deadline;
  State state;
};

}

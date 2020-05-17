#pragma once

#include <chrono>
#include <string>

namespace tgnews {

struct Document {
  Document(std::string name, std::string content, uint64_t deadline);

  std::string name;
  std::string content;
  uint64_t deadline;
};

}

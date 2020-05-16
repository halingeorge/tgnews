#include "document.h"

namespace tgnews {

  Document::Document(std::string name, std::string content, uint64_t deadline)
    : name(std::move(name)), content(std::move(content)), deadline(deadline) {}

}

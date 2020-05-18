#include "document.h"

namespace tgnews {

Document::Document(std::string name, std::string content, uint64_t deadline, State state)
    : name(std::move(name)), content(std::move(content)), deadline(deadline), state(state) {}

}  // namespace tgnews

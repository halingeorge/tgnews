#include "third_party/fastText/src/fasttext.h"

#include <optional>
#include <string>
namespace tgnews {
  std::optional<std::pair<std::string, double>> RunFasttext(const fasttext::FastText* model, const std::string& originalText, double border);
}

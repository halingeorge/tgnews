#include "run_fasttext.h"
#include <sstream>
namespace tgnews {
  std::optional<std::pair<std::string, double>> RunFasttext(const fasttext::FastText* model, const std::string& originalText, double border) {
    std::string text = originalText;
    std::replace(text.begin(), text.end(), '\n', ' ');
    std::istringstream ifs(text);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model->predictLine(ifs, predictions, 1, border);
    if (predictions.empty()) {
        return std::nullopt;
    }
    double probability = predictions[0].first;
    const size_t FT_PREFIX_LENGTH = 9; // __label__
    std::string label = predictions[0].second.substr(FT_PREFIX_LENGTH);
    return std::make_pair(label, probability);
  }
}

#include "base/document.h"
#include "base/context.h"

#include "third_party/fastText/src/fasttext.h"

#include <string>
#include <vector>

namespace tgnews {
class ParsedDoc {
 public:
  ParsedDoc(const Document& doc);
  void ParseLang(const fasttext::FastText* model);
  void Tokenize(const tgnews::Context& context);
 private:
  std::string Title;
  std::string Url;
  std::string SiteName;
  std::string Description;
  std::string Text;
  std::string Author;

  std::string GoodTitle;
  std::string GoodText;

  uint64_t PubTime = 0;
  uint64_t FetchTime = 0;

  std::vector<std::string> OutLinks;
 public:
  std::string FileName;
  std::optional<std::string> Lang;
};
}

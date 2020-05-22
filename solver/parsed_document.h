#pragma once 
#include "base/document.h"
#include "base/context.h"

#include "third_party/fastText/src/fasttext.h"

#include <string>
#include <vector>

namespace tgnews {

enum ELang {
  LangRu = 0,
  LangEn = 1,
  LangCount = 2
};

enum ENewsCategory {
    NC_NOT_NEWS = -2,
    NC_UNDEFINED = -1,
    NC_ANY = 0,
    NC_SOCIETY,
    NC_ECONOMY,
    NC_TECHNOLOGY,
    NC_SPORTS,
    NC_ENTERTAINMENT,
    NC_SCIENCE,
    NC_OTHER,

    NC_COUNT
};

class ParsedDoc {
 public:
  ParsedDoc(const Document& doc);
  void ParseLang(const fasttext::FastText* model);
  void Tokenize(const tgnews::Context& context);
  void DetectCategory(const tgnews::Context& context);
  void CalcWeight(const tgnews::Context& context);
  bool IsNews() const {
    return Category != NC_NOT_NEWS && Category != NC_UNDEFINED;
  }
  std::string Url;
  std::string SiteName;
  std::string Description;
  std::string Text;
  std::string Author;

  uint64_t PubTime = 0;
  uint64_t FetchTime = 0;

  std::vector<std::string> OutLinks;
  std::string Title;
  std::string GoodTitle;
  std::string GoodText;
  std::string FileName;
  std::optional<std::string> Lang;
  ENewsCategory Category = NC_UNDEFINED;
  fasttext::Vector Vector = fasttext::Vector(50);
  float Weight;
};

}

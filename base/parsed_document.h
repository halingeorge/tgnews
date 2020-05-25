#pragma once 
#include "base/context.h"

#include "third_party/fastText/src/fasttext.h"
#include "third_party/nlohmann_json/single_include/nlohmann/json.hpp"
#include "glog/logging.h"
#include <string>
#include <vector>
#include <limits>

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
const std::vector<std::string> CategoryNames = {"any", "society", "economy", "technology", "sports", "entertainment", "science", "other"};

class ParsedDoc {
 public:
  enum class EState {
    Added = 0,
    Removed,
    Changed
  };

 public:
  ParsedDoc(const std::string& name, std::string content, uint64_t max_age, EState state);
  ParsedDoc(const nlohmann::json& value);
  
  void ParseLang(const fasttext::FastText* model);
  void Tokenize(const Context& context);
  void DetectCategory(const Context& context);
  void CalcWeight(const Context& context);
  bool IsNews() const {
    return Category != NC_NOT_NEWS && Category != NC_UNDEFINED;
  }
  uint64_t ExpirationTime() const {
    LOG(INFO) << "fetch time: " << FetchTime;
    LOG(INFO) << "max age: " << MaxAge;
    return FetchTime + MaxAge;
  }

  nlohmann::json Serialize() const;

  std::string Data;

  EState State;

  std::string Url;
  std::string SiteName;
  std::string Description;
  std::string Text;
  std::string Author;

  uint64_t PubTime = 0;
  uint64_t FetchTime = 0;
  uint64_t MaxAge = 0;

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

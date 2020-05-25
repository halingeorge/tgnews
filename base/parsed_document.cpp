#include "parsed_document.h"

#include <third_party/tinyxml2/tinyxml2.h>
#include <glog/logging.h>

#include <boost/algorithm/string/join.hpp>
#include <ctime>
#include <regex>
#include <stdexcept>
#include <sstream>

#include "run_fasttext.h"

static uint64_t DateToTimestampFuckedup(const std::string& date) {
  std::regex ex(
      "(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)T(\\d\\d):(\\d\\d):(\\d\\d)([+-])("
      "\\d\\d):(\\d\\d)");
  std::smatch what;
  if (!std::regex_match(date, what, ex) || what.size() < 10) {
    throw std::runtime_error("wrong date format");
  }
  std::tm t = {};
  t.tm_sec = std::stoi(what[6]);
  t.tm_min = std::stoi(what[5]);
  t.tm_hour = std::stoi(what[4]);
  t.tm_mday = std::stoi(what[3]);
  t.tm_mon = std::stoi(what[2]) - 1;
  t.tm_year = std::stoi(what[1]) - 1900;

  time_t timestamp = timegm(&t);
  uint64_t zone_ts = std::stoi(what[8]) * 60 * 60 + std::stoi(what[9]) * 60;
  if (what[7] == "+") {
    timestamp = timestamp - zone_ts;
  } else if (what[7] == "-") {
    timestamp = timestamp + zone_ts;
  }
  return timestamp > 0 ? timestamp : 0;
}

static uint64_t DateToTimestamp(const std::string& date) {
  std::stringstream ss(date);
  char sep;
  std::tm t = {};
  ss >> t.tm_year;
  t.tm_year -= 1900;
  ss >> sep;
  if (sep != '-') {
    throw std::runtime_error("wrong date format");
  }
  ss >> t.tm_mon;
  t.tm_mon -= 1;
  ss >> sep;
  if (sep != '-') {
    throw std::runtime_error("wrong date format");
  }
  ss >> t.tm_mday;
  ss >> sep;
  if (sep != 'T') {
    throw std::runtime_error("wrong date format");
  }
  ss >> t.tm_hour;
  ss >> sep;
  if (sep != ':') {
    throw std::runtime_error("wrong date format");
  }
  ss >> t.tm_min;
  ss >> sep;
  if (sep != ':') {
    throw std::runtime_error("wrong date format");
  }
  ss >> t.tm_sec;
  char p;
  ss >> p;
  if (p != '+' && p != '-') {
    throw std::runtime_error("wrong date format");
  }
  int h_z, h_m;
  ss >> h_z >> sep >> h_m;
  time_t timestamp = timegm(&t);
  uint64_t zone_ts = h_z * 60 * 60 + h_m * 60;
  if (p == '+') {
    timestamp = timestamp - zone_ts;
  } else if (p == '-') {
    timestamp = timestamp + zone_ts;
  }
  return timestamp > 0 ? timestamp : 0;
}

static std::string GetFullText(const tinyxml2::XMLElement* element) {
  if (const tinyxml2::XMLText* textNode = element->ToText()) {
    return textNode->Value();
  }
  std::string text;
  const tinyxml2::XMLNode* node = element->FirstChild();
  while (node) {
    if (const tinyxml2::XMLElement* elementNode = node->ToElement()) {
      text += GetFullText(elementNode);
    } else if (const tinyxml2::XMLText* textNode = node->ToText()) {
      text += textNode->Value();
    }
    node = node->NextSibling();
  }
  return text;
}

static void ParseLinksFromText(const tinyxml2::XMLElement* element,
                               std::vector<std::string>& links) {
  const tinyxml2::XMLNode* node = element->FirstChild();
  while (node) {
    if (const tinyxml2::XMLElement* nodeElement = node->ToElement()) {
      if (std::strcmp(nodeElement->Value(), "a") == 0 &&
          nodeElement->Attribute("href")) {
        links.push_back(nodeElement->Attribute("href"));
      }
      ParseLinksFromText(nodeElement, links);
    }
    node = node->NextSibling();
  }
}

namespace tgnews {

ParsedDoc::ParsedDoc(Context* context, const std::string& name, std::string content,
                     uint64_t max_age, EState state)
    : Data(std::move(content)), MaxAge(max_age), State(state) {
  FileName = name;

  tinyxml2::XMLDocument originalDoc;
  originalDoc.Parse(Data.data());
  const tinyxml2::XMLElement* htmlElement =
      originalDoc.FirstChildElement("html");
  if (!htmlElement) {
    throw std::runtime_error("Parser error: no html tag");
  }
  const tinyxml2::XMLElement* headElement =
      htmlElement->FirstChildElement("head");
  if (!headElement) {
    throw std::runtime_error("Parser error: no head");
  }
  const tinyxml2::XMLElement* metaElement =
      headElement->FirstChildElement("meta");
  if (!metaElement) {
    throw std::runtime_error("Parser error: no meta");
  }
  while (metaElement != 0) {
    const char* property = metaElement->Attribute("property");
    const char* content = metaElement->Attribute("content");
    if (content == nullptr || property == nullptr) {
      metaElement = metaElement->NextSiblingElement("meta");
      continue;
    }
    if (std::strcmp(property, "og:title") == 0) {
      Title = content;
    }
    if (std::strcmp(property, "og:url") == 0) {
      Url = content;
    }
    if (std::strcmp(property, "og:site_name") == 0) {
      SiteName = content;
    }
    if (std::strcmp(property, "og:description") == 0) {
      Description = content;
    }
    if (std::strcmp(property, "article:published_time") == 0) {
      FetchTime = DateToTimestamp(content);
    }
    metaElement = metaElement->NextSiblingElement("meta");
  }
  const tinyxml2::XMLElement* bodyElement =
      htmlElement->FirstChildElement("body");
  if (!bodyElement) {
    throw std::runtime_error("Parser error: no body");
  }
  const tinyxml2::XMLElement* articleElement =
      bodyElement->FirstChildElement("article");
  if (!articleElement) {
    throw std::runtime_error("Parser error: no article");
  }
  const tinyxml2::XMLElement* pElement = articleElement->FirstChildElement("p");
  {
    size_t wordCount = 0;
    while (pElement) {
      Text += GetFullText(pElement) + "\n";
      pElement = pElement->NextSiblingElement("p");
    }
  }
  const tinyxml2::XMLElement* addressElement =
      articleElement->FirstChildElement("address");
  if (!addressElement) {
    return;
  }
  const tinyxml2::XMLElement* timeElement =
      addressElement->FirstChildElement("time");
  if (timeElement && timeElement->Attribute("datetime")) {
    PubTime = DateToTimestamp(timeElement->Attribute("datetime"));
  }
  const tinyxml2::XMLElement* aElement = addressElement->FirstChildElement("a");
  if (aElement && aElement->Attribute("rel") &&
      std::string(aElement->Attribute("rel")) == "author") {
    Author = aElement->GetText();
  }
  ParseLang(context->LangDetect.get());
  Tokenize(*context);
  DetectCategory(*context);
  CalcWeight(*context);
}

ParsedDoc::ParsedDoc(const nlohmann::json& value) : State(EState::Added) {
#define GET(s) value.at(#s).get_to(s);
  GET(Title);
  GET(Url);
  GET(SiteName);
  GET(Description);
  GET(Text);
  GET(FileName);
  GET(FetchTime);
  GET(MaxAge);
  GET(Lang);
  GET(GoodTitle);
  GET(GoodText);
  GET(Category);
  GET(Weight);
#undef GET
  State = EState::Added;
}
nlohmann::json ParsedDoc::Serialize() const {
  nlohmann::json res;
#define ADD(s) res[#s] = s;
  ADD(Title);
  ADD(Url);
  ADD(SiteName);
  ADD(Description);
  ADD(FetchTime);
  ADD(FileName);
  ADD(Text);
  ADD(MaxAge);
  ADD(Lang);
  ADD(GoodTitle);
  ADD(GoodText);
  ADD(Category);
  ADD(Weight);
#undef ADD
  return res;
}

void ParsedDoc::ParseLang(const fasttext::FastText* model) {
  if (Lang.size()) {
    // already parsed - skip
    return;
  }
  std::string sample(Title + " " + Description + " " + Text.substr(0, 100));
  auto pair = RunFasttext(model, sample, 0.4);
  if (!pair) {
    Lang = "";
    return;
  }
  const std::string& label = pair->first;
  double probability = pair->second;
  if ((label == "ru") && probability < 0.6) {
    Lang = std::string("tg");
  } else {
    Lang = label;
  }
}

static std::string Preprocess(const std::string& text,
                              const onmt::Tokenizer& tokenizer) {
  std::vector<std::string> tokens;
  tokenizer.tokenize(text, tokens);
  return boost::join(tokens, " ");
}

void ParsedDoc::Tokenize(const tgnews::Context& context) {
  if (GoodTitle.size() > 0 && GoodText.size()) {
    return; // already calced
  }
  GoodTitle = Preprocess(Title, context.Tokenizer);
  GoodText = Preprocess(Text, context.Tokenizer);
}

void ParsedDoc::DetectCategory(const tgnews::Context& context) {
  if (Category != NC_UNDEFINED) {
    return; //allready calced
  }
  if (!GoodTitle.size() || !GoodText.size() || !Lang.size()) {
    Category = NC_UNDEFINED;
    return;
  }
  if (Lang != "ru" & Lang != "en") {
    Category = NC_UNDEFINED;
    return;
  }
  std::string sample(GoodTitle + " " + GoodText);
  const fasttext::FastText* model =
      Lang == "ru" ? context.RuCatModel.get() : context.EnCatModel.get();
  auto pair = RunFasttext(model, sample, 0.0);
  if (!pair) {
    Category = NC_UNDEFINED;
    return;
  }
  const std::string& label = pair->first;
  if (label == "any") {
    Category = NC_ANY;
  } else if (label == "society") {
    Category = NC_SOCIETY;
  } else if (label == "economy") {
    Category = NC_ECONOMY;
  } else if (label == "technology") {
    Category = NC_TECHNOLOGY;
  } else if (label == "sports") {
    Category = NC_SPORTS;
  } else if (label == "entertainment") {
    Category = NC_ENTERTAINMENT;
  } else if (label == "science") {
    Category = NC_SCIENCE;
  } else if (label == "other") {
    Category = NC_OTHER;
  } else if (label == "not_news") {
    Category = NC_NOT_NEWS;
  } else {
    Category = NC_UNDEFINED;
  }
  return;
}

void ParsedDoc::CalcWeight(const tgnews::Context& context) {
  if (Weight != -1.f) {
    return; // allready calced
  }
  Weight = context.Ratings.ScoreUrl(Url);
}

}  // namespace tgnews

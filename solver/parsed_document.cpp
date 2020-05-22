#include "parsed_document.h"
#include "run_fasttext.h"

#include <boost/algorithm/string/join.hpp>
#include <third_party/tinyxml2/tinyxml2.h>

#include <stdexcept>
#include <ctime>
#include <regex>

static uint64_t DateToTimestamp(const std::string& date) {
  std::regex ex("(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)T(\\d\\d):(\\d\\d):(\\d\\d)([+-])(\\d\\d):(\\d\\d)");
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

static void ParseLinksFromText(const tinyxml2::XMLElement* element, std::vector<std::string>& links) {
  const tinyxml2::XMLNode* node = element->FirstChild();
  while (node) {
    if (const tinyxml2::XMLElement* nodeElement = node->ToElement()) {
      if (std::strcmp(nodeElement->Value(), "a") == 0 && nodeElement->Attribute("href")) {
        links.push_back(nodeElement->Attribute("href"));
      }
      ParseLinksFromText(nodeElement, links);
    }
    node = node->NextSibling();
  }
}

namespace tgnews {

ParsedDoc::ParsedDoc(const Document& doc) : ParsedDoc(doc.name, doc.content) {}

ParsedDoc::ParsedDoc(const std::string& name, const std::string& content) {
  FileName = name;

  tinyxml2::XMLDocument originalDoc;
  originalDoc.Parse(content.data());
  const tinyxml2::XMLElement* htmlElement = originalDoc.FirstChildElement("html");
  if (!htmlElement) {
    throw std::runtime_error("Parser error: no html tag");
  }
  const tinyxml2::XMLElement* headElement = htmlElement->FirstChildElement("head");
  if (!headElement) {
    throw std::runtime_error("Parser error: no head");
  }
  const tinyxml2::XMLElement* metaElement = headElement->FirstChildElement("meta");
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
  const tinyxml2::XMLElement* bodyElement = htmlElement->FirstChildElement("body");
  if (!bodyElement) {
    throw std::runtime_error("Parser error: no body");
  }
  const tinyxml2::XMLElement* articleElement = bodyElement->FirstChildElement("article");
  if (!articleElement) {
    throw std::runtime_error("Parser error: no article");
  }
  const tinyxml2::XMLElement* pElement = articleElement->FirstChildElement("p");
  {
    std::vector<std::string> links;
    size_t wordCount = 0;
    while (pElement) {
      Text += GetFullText(pElement) + "\n";
      ParseLinksFromText(pElement, links);
      pElement = pElement->NextSiblingElement("p");
    }
    OutLinks = std::move(links);
  }
  const tinyxml2::XMLElement* addressElement = articleElement->FirstChildElement("address");
  if (!addressElement) {
    return;
  }
  const tinyxml2::XMLElement* timeElement = addressElement->FirstChildElement("time");
  if (timeElement && timeElement->Attribute("datetime")) {
    PubTime = DateToTimestamp(timeElement->Attribute("datetime"));
  }
  const tinyxml2::XMLElement* aElement = addressElement->FirstChildElement("a");
  if (aElement && aElement->Attribute("rel") && std::string(aElement->Attribute("rel")) == "author") {
    Author = aElement->GetText();
  }
}

ParsedDoc::ParsedDoc(const Json::Value& value) {
#define GET_STRING(s) s = value[#s].asString();
  GET_STRING(Title);
  GET_STRING(Url);
  GET_STRING(SiteName);
  GET_STRING(Description);
  GET_STRING(Text);
#undef GET_STRING
#define GET_UINT64(s) s = value[#s].asUInt64();
  GET_UINT64(FetchTime);
#undef GET_UINT64
}
Json::Value ParsedDoc::Serialize() const {
  Json::Value res;
#define ADD(s) res[#s] = s;
  ADD(Title);
  ADD(Url);
  ADD(SiteName);
  ADD(Description);
  ADD(FetchTime);
  ADD(Text);
#undef ADD
  return res;
}


void ParsedDoc::ParseLang(const fasttext::FastText* model) {
  std::string sample(Title + " " + Description + " " + Text.substr(0, 100));
  auto pair = RunFasttext(model, sample, 0.4);
  if (!pair) {
    Lang = std::nullopt;
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

static std::string Preprocess(const std::string& text, const onmt::Tokenizer& tokenizer) {
  std::vector<std::string> tokens;
  tokenizer.tokenize(text, tokens);
  return boost::join(tokens, " ");
}

void ParsedDoc::Tokenize(const tgnews::Context& context) {
  GoodTitle = Preprocess(Title, context.Tokenizer);
  GoodText = Preprocess(Text, context.Tokenizer);
}

void ParsedDoc::DetectCategory(const tgnews::Context& context) {
  if (!GoodTitle.size() || !GoodText.size() || !Lang) {
    Category = NC_UNDEFINED;
    return;
  }
  if (Lang != "ru" & Lang != "en") {
    Category = NC_UNDEFINED;
    return;
  }
  std::string sample(GoodTitle + " " + GoodText);
  const fasttext::FastText* model = Lang == "ru" ? context.RuCatModel.get() : context.EnCatModel.get();
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
  Weight = context.Ratings.ScoreUrl(Url);
}

}

#include "response_builder.h"

#include <vector>
#include <string>

namespace {

Json::Value CalcLangAns(const std::vector<tgnews::ParsedDoc>& docs) {
  Json::Value result;
  int idx = 0;
  for (const auto& lang : {"en", "ru"}) {
    result[idx]["lang_code"] = lang;
    int docidx = 0;
    for (const auto& doc : docs) {
      if (doc.Lang == lang) {
        result[idx]["articles"][docidx] = doc.FileName;
        ++docidx;
      }
    }
    ++idx;
  }
  return result;
}

}

namespace tgnews {

CalculatedResponses::CalculatedResponses(const std::vector<tgnews::ParsedDoc>& docs) {
  LangAns = CalcLangAns(docs);
}

Json::Value CalculatedResponses::GetAns(const std::string& lang, const std::string& category, const uint64_t period) {
  return Json::Value{};
}

ResponseBuilder::ResponseBuilder(tgnews::Context context) : Context(std::move(context)) {}

CalculatedResponses ResponseBuilder::AddDocuments(const std::vector<std::shared_ptr<const tgnews::Document>>& docs) {
  for (const auto& doc : docs) {
    Docs.emplace_back(*doc);
  }

  for (auto& doc : Docs) {
    doc.ParseLang(Context.langDetect.get());
  }

  return {Docs};
}

}

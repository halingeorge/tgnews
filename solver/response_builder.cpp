#include "response_builder.h"

#include <vector>
#include <string>

namespace tgnews {

CalculatedResponses::CalculatedResponses(const std::vector<tgnews::ParsedDoc>& docs) {
  int idx = 0;
  for (const auto& lang : {"en", "ru"}) {
    Ans[idx]["lang_code"] = lang;
    ++idx;
    int docidx = 0;
    for (const auto& doc : docs) {
      if (doc.Lang == lang) {
        Ans[idx]["articles"][docidx] = doc.FileName;
        ++docidx;
      }
    }
  }
}

Json::Value CalculatedResponses::GetAns(const std::string& lang, const std::string& category, const uint64_t period) {
  return Ans;
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

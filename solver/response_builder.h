#include "parsed_document.h"

#include "server/context.h"
#include "base/document.h"

#include "json/json.h"

#include <memory>

namespace tgnews {

class CalculatedResponses {
 public:
  CalculatedResponses(const std::vector<tgnews::ParsedDoc>& docs);
  Json::Value GetAns(const std::string& lang = {}, const std::string& category = {}, const uint64_t period = 0);
 private:
  Json::Value Ans;
};

class ResponseBuilder {
 public:
  ResponseBuilder(tgnews::Context context);
  CalculatedResponses AddDocuments(const std::vector<std::unique_ptr<tgnews::Document>>& docs);
 private:

  std::vector<tgnews::ParsedDoc> Docs;
  tgnews::Context Context;
};

}

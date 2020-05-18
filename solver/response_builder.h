#include "parsed_document.h"

#include "base/context.h"
#include "base/document.h"

#include "json/json.h"

#include <memory>

namespace tgnews {

class CalculatedResponses {
 public:
  CalculatedResponses(const std::vector<tgnews::ParsedDoc>& docs);
  Json::Value GetAns(const std::string& lang = {}, const std::string& category = {}, const uint64_t period = 0);
 public:
  Json::Value LangAns;
  Json::Value NewsAns;
  Json::Value CategoriesAns;
  Json::Value ThreadsAns;
};

class ResponseBuilder {
 public:
  ResponseBuilder(tgnews::Context context);
  CalculatedResponses AddDocuments(const std::vector<DocumentConstPtr>& docs);

 private:
  std::vector<tgnews::ParsedDoc> Docs;
  tgnews::Context Context;
};

}

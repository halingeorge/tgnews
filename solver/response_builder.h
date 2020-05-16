#include "server/context.h"
#include "base/document.h"

#include "json/json.h"

namespace tgnews {

  class CalculatedResponses {
  public:
    Json::Value GetAns(const std::string& lang, const std::string& category, const uint64_t period);
  };

  class ResponseBuilder {
  public:
    ResponseBuilder(tgnews::Context* context);
    CalculatedResponses AddDocuments(const std::vector<tgnews::Document*>& docs);
  private:

    std::vector<tgnews::Document*> Docs;
    tgnews::Context* Context;
  };

}

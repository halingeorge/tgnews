#include "response_builder.h"

namespace tgnews {

  ResponseBuilder::ResponseBuilder(tgnews::Context* context) : Context(context) {}

  CalculatedResponses ResponseBuilder::AddDocuments(const std::vector<tgnews::Document*>& docs) {
    for (const auto* doc : docs) {
      Docs.emplace_back(*doc);
    }
    
  }

}

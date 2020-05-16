#include "response_builder.h"

namespace tgnews {

  ResponseBuilder::ResponseBuilder(tgnews::Context* context) : Context(context) {}

  CalculatedResponses ResponseBuilder::AddDocuments(const std::vector<tgnews::Document*>& docs) {
    std::copy(docs.begin(), docs.end(), std::back_inserter(Docs));
  }

}

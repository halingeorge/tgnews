#include "context.h"

namespace tgnews {

  class CalculatedResponses {
  public:
    uint32_t tmp;
  };

  class ResponseBuilder {
  public:
    ResponseBuilder(tgnews::Context* context);
    CalculatedResponses AddDocuments(const std::vector<tgnews::Document>& docs);
  private:
    tgnews::Context* Context;
  };

}

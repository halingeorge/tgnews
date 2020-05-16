#include "server/document.h"

#include <string>
#include <vector>

namespace tgnews {
  class ParsedDoc {
  public:
    ParsedDoc(const Document& doc);

  private:
    std::string Title;
    std::string Url;
    std::string SiteName;
    std::string Description;
    std::string FileName;
    std::string Text;
    std::string Author;

    uint64_t PubTime = 0;
    uint64_t FetchTime = 0;

    std::vector<std::string> OutLinks;
  };
}

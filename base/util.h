#include <vector>
#include <memory>

#include "base/parsed_document.h"

namespace tgnews {

std::string GetHost(const std::string& url);

std::vector<ParsedDoc> MakeDocumentsFromDir(const std::string& dir, int nDocs = -1);

}

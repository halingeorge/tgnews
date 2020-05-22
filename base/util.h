#include "base/document.h"

#include <vector>
#include <memory>

namespace tgnews {

std::string GetHost(const std::string& url);

std::vector<DocumentConstPtr> MakeDocumentsFromDir(const std::string& dir, int nDocs = -1);

}

#include "base/document.h"

#include <vector>
#include <memory>

namespace tgnews {

std::vector<DocumentConstPtr> MakeDocumentsFromDir(const std::string& dir);

}

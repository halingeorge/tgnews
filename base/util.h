#include "base/document.h"

#include <vector>
#include <memory>

namespace tgnews {

std::vector<std::unique_ptr<tgnews::Document>> MakeDocumentsFromDir(const std::string& dir);

}

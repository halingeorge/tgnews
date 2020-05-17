#include "util.h"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace tgnews {

std::vector<std::unique_ptr<tgnews::Document>> MakeDocumentsFromDir(const std::string& dir) {
  boost::filesystem::path path = dir;
  std::vector<std::unique_ptr<tgnews::Document>> documents;
  for (const auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {})) {
    boost::filesystem::ifstream file(entry.path());
    std::string content;
    file >> content;
    documents.emplace_back(new tgnews::Document(entry.path().filename().string(), std::move(content), 0));
  }
  return documents;
}

}

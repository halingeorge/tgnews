#include "util.h"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include <fstream>

namespace tgnews {

std::vector<DocumentConstPtr> MakeDocumentsFromDir(const std::string& dir) {
  boost::filesystem::path path = dir;
  std::vector<DocumentConstPtr> documents;
  for (const auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {})) {
    std::ifstream file(entry.path().string());
    std::string content(std::istreambuf_iterator<char>(file), {});
    documents.emplace_back(new tgnews::Document(entry.path().filename().string(), std::move(content), 0, tgnews::Document::State::Added));
  }
  return documents;
}

}

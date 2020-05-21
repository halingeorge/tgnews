#include "util.h"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include <fstream>

namespace tgnews {

std::vector<DocumentConstPtr> MakeDocumentsFromDir(const std::string& dir, int nDocs) {
  boost::filesystem::path dirPath(dir);
  boost::filesystem::recursive_directory_iterator start(dirPath);
  boost::filesystem::recursive_directory_iterator end;
  std::vector<DocumentConstPtr> documents;

  for (auto it = start; it != end; it++) {
    if (boost::filesystem::is_directory(it->path())) {
      continue;
    }
    std::string path = it->path().string();
    if (path.substr(path.length() - 5) == ".html") {
      std::ifstream file(path);
      std::string content(std::istreambuf_iterator<char>(file), {});
      documents.emplace_back(new tgnews::Document(it->path().filename().string(), std::move(content), 0, tgnews::Document::State::Added));
    }
    if (nDocs != -1 && documents.size() == static_cast<size_t>(nDocs)) {
      break;
    }
  }
  return documents;
}

}


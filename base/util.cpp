#include "util.h"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include <fstream>
#include <regex>

namespace tgnews {

std::string GetHost(const std::string& url) {
  std::string output = "";
  try {
    std::regex ex("(http|https)://(?:www\\.)?([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
    std::smatch what;
    if (std::regex_match(url, what, ex) && what.size() >= 3) {
      output = std::string(what[2].first, what[2].second);
    }
  } catch (...) {
    return output;
  }
  return output;
}


std::vector<ParsedDoc> MakeDocumentsFromDir(const std::string& dir, int nDocs) {
  boost::filesystem::path dirPath(dir);
  boost::filesystem::recursive_directory_iterator start(dirPath);
  boost::filesystem::recursive_directory_iterator end;
  std::vector<ParsedDoc> documents;

  for (auto it = start; it != end; it++) {
    if (boost::filesystem::is_directory(it->path())) {
      continue;
    }
    std::string path = it->path().string();
    if (path.substr(path.length() - 5) == ".html") {
      std::ifstream file(path);
      std::string content(std::istreambuf_iterator<char>(file), {});
      documents.emplace_back(ParsedDoc(it->path().filename().string(), std::move(content), 0, ParsedDoc::EState::Added));
    }
    if (nDocs != -1 && documents.size() == static_cast<size_t>(nDocs)) {
      break;
    }
  }
  return documents;
}

}


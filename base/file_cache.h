#pragma once

#include <string>
#include <vector>

namespace tgnews {

class ParsedDoc;
class FileManager;

class FileCache {
 public:
  explicit FileCache(FileManager* file_manager);

  bool IsFileStillAlive(std::string filename);

  std::vector<ParsedDoc> GetDocuments();

 private:
  FileManager* const file_manager_;
};

}  // namespace tgnews

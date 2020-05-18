#pragma once

#include "base/document.h"
#include "base/file_manager.h"

namespace tgnews {

class FileCache {
 public:
  explicit FileCache(FileManager* file_manager) : file_manager_(file_manager) {
  }

  bool IsFileStillAlive(std::string_view filename) {
    return file_manager_->IsFileStillAlive(filename);
  }

  std::vector<std::shared_ptr<const Document>> GetDocuments() {
    return file_manager_->GetDocuments();
  }

 private:
  FileManager* const file_manager_;
};

}  // namespace tgnews

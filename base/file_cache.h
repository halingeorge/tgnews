#pragma once

#include "base/document.h"
#include "base/file_manager.h"

namespace tgnews {

class FileCache {
 public:
  explicit FileCache(FileManager* file_manager) : file_manager_(file_manager) {
  }

  bool IsFileStillAlive(std::string filename) {
    return file_manager_->IsFileStillAlive(std::move(filename));
  }

  std::vector<DocumentConstPtr> GetDocuments() {
    return file_manager_->GetDocuments().apply(cti::transforms::wait());
  }

 private:
  FileManager* const file_manager_;
};

}  // namespace tgnews

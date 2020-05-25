#include "base/file_cache.h"

#include "base/file_manager.h"

namespace tgnews {

FileCache::FileCache(FileManager* file_manager) : file_manager_(file_manager) {
}

std::vector<ParsedDoc> FileCache::GetDocuments() {
  return file_manager_->GetDocuments().apply(cti::transforms::wait());
}

}  // namespace tgnews

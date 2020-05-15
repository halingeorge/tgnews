#include "server/file_manager.h"

namespace tgnews {

class FileCache {
 public:
  explicit FileCache(FileManager& file_manager) : file_manager_(file_manager) {
  }

  bool IsFileStillAlive(std::string_view filename) {
    return file_manager_.IsFileStillAlive(filename);
  }

 private:
  FileManager& file_manager_;
};

}  // namespace tgnews
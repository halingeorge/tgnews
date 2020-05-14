#pragma once

#include <boost/filesystem/fstream.hpp>
#include <limits>
#include <string>

#include "glog/logging.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace tgnews {

class FileManager {
 public:
  FileManager() {}

  void StoreFile(std::string filename, std::string content,
                 uint64_t max_age = std::numeric_limits<uint64_t>::max()) {
    Json::Value value;
    value["content"] = std::move(content);
    value["max_age"] = max_age;

    LOG(INFO) << "write json to file: " << value;

    boost::filesystem::ofstream file(filename);
    file << value;
    file.close();

    LOG(INFO) << "written on disk: " << filename;
  }
};

}  // namespace tgnews
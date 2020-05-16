#pragma once

#include "server/base.h"
#include "server/document.h"

#include "glog/logging.h"
#include "json/json.h"
#include "fmt/format.h"

#include <optional>
#include <string>
#include <chrono>
#include <limits>
#include <map>
#include <memory>
#include <unordered_map>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace tgnews {


class FileManager {
 public:
  FileManager() {
    RestoreFiles();
  }

  bool StoreOrUpdateFile(std::string filename, std::string content, uint64_t max_age) {
    auto deadline = std::chrono::time_point<std::chrono::seconds>().time_since_epoch().count() + max_age;
    std::string filepath = fmt::format("{0}/{1}", kContentDir, filename);

    auto it = document_by_name_.find(filename);
    if (it != document_by_name_.end()) {
      auto* document = it->second;

      document->content = std::move(content);
      document->deadline = deadline;

      DumpOnDisk(filepath, *document);

      return true;
    }

    DumpOnDisk(filepath, CreateDocument(std::move(filename), std::move(content), deadline));

    return false;
  }

  bool RemoveFile(std::string_view filename) {
    auto it = document_by_name_.find(filename.data());
    if (it == document_by_name_.end()) {
      return false;
    }
    boost::filesystem::path filepath = fmt::format("{0}/{1}", kContentDir, it->second->name);
    RemoveFileFromDisk(filepath);
    document_by_deadline_.erase(document_by_deadline_.find(it->second->deadline));
    document_by_name_.erase(it);
    return true;
  }

  void RemoveOutdatedFiles() {
    auto now = std::chrono::time_point<std::chrono::seconds>().time_since_epoch().count();

    for (auto it = document_by_deadline_.begin(); it != document_by_deadline_.end(); ++it) {
      if (it->first > now) {
        break;
      }

      LOG(INFO) << "file is outdated: " << it->second->name;

      boost::filesystem::path filepath = fmt::format("{0}/{1}", kContentDir, it->second->name);

      RemoveFileFromDisk(filepath);

      document_by_deadline_.erase(it);
      document_by_name_.erase(it->second->name);
    }
  }

  bool IsFileStillAlive(std::string_view filename) {
    auto now = std::chrono::time_point<std::chrono::seconds>().time_since_epoch().count();

    auto it = document_by_name_.find(filename.data());

    VERIFY(it != document_by_name_.end(), fmt::format("no file found for aliveness check: {0}", filename));

    LOG(INFO) << "file still alive: " << filename;

    return it->second->deadline > now;
  }

 private:
  Document& CreateDocument(std::string filename, std::string content, uint64_t deadline) {
    auto document_ptr = std::make_unique<Document>(std::move(filename), std::move(content), deadline);

    auto address = document_ptr.get();

    LOG(INFO) << "create document: " << document_ptr->name;

    document_by_name_.emplace(document_ptr->name, document_ptr.get());
    document_by_deadline_.emplace(deadline, std::move(document_ptr));

    return *address;
  }

  void DumpOnDisk(std::string_view filepath, const Document& document) {
    Json::Value value;

    value["deadline"] = document.deadline;
    value["content"] = document.content;

    LOG(INFO) << "write json to file: " << value;

    boost::filesystem::ofstream file(filepath.data());
    file << value;
    file.close();

    LOG(INFO) << "written on disk: " << filepath;
  }

  void RestoreFiles() {
    auto now = std::chrono::time_point<std::chrono::seconds>().time_since_epoch().count();

    boost::filesystem::path path = kContentDir;
    if (!boost::filesystem::exists(path)) {
      return;
    }

    for (const auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {})) {
      boost::filesystem::ifstream file(entry.path());

      Json::Value value;
      file >> value;

      auto deadline = value["deadline"].as<uint64_t>();
      auto content = value["content"].as<std::string>();

      if (deadline <= now) {
        LOG(INFO) << "file is outdated: " << entry.path();
        RemoveFileFromDisk(entry.path());
        continue;
      }

      CreateDocument(entry.path().filename().string(), std::move(content), deadline);
    }

    LOG(INFO) << "restored file count: " << document_by_deadline_.size();
  }

  static void RemoveFileFromDisk(const boost::filesystem::path& filepath) {
    LOG(INFO) << fmt::format("removing file from disk: {0}", filepath.string());
    VERIFY(boost::filesystem::remove(filepath), fmt::format("no file found: {0}", filepath.string()));
  }

 private:
  static constexpr const char* kContentDir = "content";

  std::multimap<uint64_t, std::unique_ptr<Document>> document_by_deadline_;
  std::unordered_map<std::string, Document*> document_by_name_;
};

}  // namespace tgnews

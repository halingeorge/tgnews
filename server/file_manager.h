#pragma once

#include "server/base.h"
#include "base/document.h"

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
#include <vector>
#include <set>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace tgnews {

class FileManager {
 public:
  explicit FileManager(std::string content_dir = "content") : content_dir_(std::move(content_dir)) {
    boost::filesystem::path content_path = content_dir_;
    VERIFY(boost::filesystem::exists(content_path), fmt::format("content path does not exist: {}", content_dir));
    RestoreFiles();
  }

  bool StoreOrUpdateFile(std::string filename, std::string content, uint64_t max_age) {
    auto deadline = std::chrono::time_point<std::chrono::seconds>().time_since_epoch().count() + max_age;
    std::string filepath = fmt::format("{0}/{1}", content_dir_, filename);

    auto it = document_by_name_.find(filename);
    if (it != document_by_name_.end()) {
      auto* document = it->second.get();

      documents_with_deadline_.erase({document->deadline, document});

      document->content = std::move(content);
      document->deadline = deadline;

      documents_with_deadline_.emplace(document->deadline, document);

      DumpOnDisk(filepath, *document);

      return true;
    }

    DumpOnDisk(filepath, CreateDocument(std::move(filename), std::move(content), deadline));

    return false;
  }

  bool RemoveFile(std::string_view filename) {
    LOG(INFO) << "removing file: " << filename;
    auto it = document_by_name_.find(filename.data());
    if (it == document_by_name_.end()) {
      return false;
    }
    LOG(INFO) << "content dir: " << content_dir_;
    LOG(INFO) << "name: " << it->second->name;
    boost::filesystem::path filepath = fmt::format("{0}/{1}", content_dir_, it->second->name);
    RemoveFileFromDisk(filepath);
    documents_with_deadline_.erase({it->second->deadline, it->second.get()});
    document_by_name_.erase(it);
    return true;
  }

  void RemoveOutdatedFiles() {
    auto now = std::chrono::time_point<std::chrono::seconds>().time_since_epoch().count();

    for (auto it = documents_with_deadline_.begin(); it != documents_with_deadline_.end(); ++it) {
      if (it->first > now) {
        break;
      }

      LOG(INFO) << "file is outdated: " << it->second->name;

      boost::filesystem::path filepath = fmt::format("{0}/{1}", content_dir_, it->second->name);

      RemoveFileFromDisk(filepath);

      document_by_name_.erase(it->second->name);
      documents_with_deadline_.erase(it);
    }
  }

  bool IsFileStillAlive(std::string_view filename) {
    auto now = std::chrono::time_point<std::chrono::seconds>().time_since_epoch().count();

    auto it = document_by_name_.find(filename.data());

    VERIFY(it != document_by_name_.end(), fmt::format("no file found for aliveness check: {0}", filename));

    LOG(INFO) << "file still alive: " << filename;

    return it->second->deadline > now;
  }

  std::vector<Document*> GetDocuments() {
    std::vector<Document*> documents;
    documents.reserve(document_by_name_.size());
    for (auto&[name, document_ptr] : document_by_name_) {
      documents.push_back(document_ptr.get());
    }
    return documents;
  }

 private:
  Document& CreateDocument(std::string filename, std::string content, uint64_t deadline) {
    auto document_ptr = std::make_unique<Document>(std::move(filename), std::move(content), deadline);

    auto address = document_ptr.get();

    LOG(INFO) << "create document: " << document_ptr->name;

    documents_with_deadline_.emplace(deadline, address);
    document_by_name_.emplace(document_ptr->name, std::move(document_ptr));

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

    boost::filesystem::path path = filepath.data();
    VERIFY(boost::filesystem::exists(path), "file has to be on disk after it's written");

    LOG(INFO) << "written on disk: " << filepath;
  }

  void RestoreFiles() {
    auto now = std::chrono::time_point<std::chrono::seconds>().time_since_epoch().count();

    boost::filesystem::path path = content_dir_;
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

    LOG(INFO) << "restored file count: " << document_by_name_.size();
  }

  static void RemoveFileFromDisk(const boost::filesystem::path& filepath) {
    LOG(INFO) << fmt::format("removing file from disk: {0}", filepath.string());
    VERIFY(boost::filesystem::remove(filepath), fmt::format("no file found: {0}", filepath.string()));
  }

 private:
  std::string content_dir_;
  std::unordered_map<std::string, std::unique_ptr<Document>> document_by_name_;
  std::set<std::pair<uint64_t, Document*>> documents_with_deadline_;
};

}  // namespace tgnews

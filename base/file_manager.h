#pragma once

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <chrono>
#include <continuable/continuable.hpp>
#include <experimental/strand>
#include <experimental/thread_pool>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/base.h"
#include "base/document.h"
#include "base/time_helpers.h"
#include "fmt/format.h"
#include "glog/logging.h"
#include "json/json.h"

namespace tgnews {

class FileManager {
 public:
  explicit FileManager(std::experimental::thread_pool& pool,
                       std::string content_dir = "content")
      : content_dir_(std::move(content_dir)), pool_(pool) {
    boost::filesystem::path content_path = content_dir_;
    VERIFY(boost::filesystem::exists(content_path),
           fmt::format("content path does not exist: {}", content_dir));
    RestoreFiles();
  }

  bool StoreOrUpdateFile(std::string filename, std::string content,
                         uint64_t max_age) {
    auto deadline = DeadlineCount(std::chrono::seconds(max_age));
    std::string filepath = fmt::format("{0}/{1}", content_dir_, filename);

    auto it = document_by_name_.find(filename);
    if (it != document_by_name_.end()) {
      auto* document = it->second.get();

      documents_with_deadline_.erase({document->deadline, document});

      document->content = std::move(content);
      document->deadline = deadline;
      document->state = Document::State::Changed;

      documents_with_deadline_.emplace(document->deadline, document);

      DumpOnDisk(filepath, *document);

      return true;
    }

    DumpOnDisk(filepath, CreateDocument(std::move(filename), std::move(content),
                                        deadline));

    return false;
  }

  bool RemoveFile(std::string filename) {
    RemoveFileFromDisk(filename);
    return RemoveFileFromMap(filename);
  }

  void RemoveOutdatedFiles() {
    auto now = NowCount();

    for (auto it = documents_with_deadline_.begin();
         it != documents_with_deadline_.end(); ++it) {
      if (it->first > now) {
        break;
      }
      RemoveFileFromDisk(it->second->name);
      RemoveFileFromMap(it->second->name);
    }
  }

  bool IsFileStillAlive(std::string_view filename) {
    auto now = NowCount();
    auto it = document_by_name_.find(filename.data());
    VERIFY(it != document_by_name_.end(),
           fmt::format("no file found for aliveness check: {0}", filename));
    LOG(INFO) << "file still alive: " << filename;
    return it->second->deadline > now;
  }

  std::vector<DocumentConstPtr> GetDocuments() {
    std::vector<DocumentConstPtr> documents;
    documents.reserve(document_by_name_.size());
    for (auto& [name, document_ptr] : document_by_name_) {
      documents.push_back(document_ptr);
    }
    return documents;
  }

 private:
  Document& CreateDocument(std::string filename, std::string content,
                           uint64_t deadline) {
    auto document_ptr =
        std::make_unique<Document>(std::move(filename), std::move(content),
                                   deadline, Document::State::Added);

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
    VERIFY(boost::filesystem::exists(path),
           "file has to be on disk after it's written");

    LOG(INFO) << "written on disk: " << filepath;
  }

  void RestoreFiles() {
    auto now = std::chrono::time_point<std::chrono::seconds>()
                   .time_since_epoch()
                   .count();

    boost::filesystem::path path = content_dir_;
    for (const auto& entry : boost::make_iterator_range(
             boost::filesystem::directory_iterator(path), {})) {
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

      CreateDocument(entry.path().filename().string(), std::move(content),
                     deadline);
    }

    LOG(INFO) << "restored file count: " << document_by_name_.size();
  }

  bool RemoveFileFromMap(std::string filename) {
    auto it = document_by_name_.find(filename);
    if (it == document_by_name_.end()) {
      return false;
    }

    it->second->state = Document::State::Removed;
    documents_with_deadline_.erase({it->second->deadline, it->second.get()});
    document_by_name_.erase(it);
    return true;
  }

  static void RemoveFileFromDisk(const boost::filesystem::path& filepath) {
    LOG(INFO) << fmt::format("removing file from disk {0}: {1}",
                             filepath.string(),
                             boost::filesystem::remove(filepath));
  }

 private:
  std::string content_dir_;
  std::unordered_map<std::string, std::shared_ptr<Document>> document_by_name_;
  std::set<std::pair<uint64_t, Document*>> documents_with_deadline_;
  std::experimental::thread_pool& pool_;
};

}  // namespace tgnews

#pragma once

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <chrono>
#include <continuable/continuable.hpp>
#include <experimental/future>
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
#include "base/parsed_document.h"
#include "base/time_helpers.h"
#include "fmt/format.h"
#include "glog/logging.h"
#include "third_party/nlohmann_json/single_include/nlohmann/json.hpp"

namespace tgnews {

class FileManager {
 public:
  explicit FileManager(std::experimental::thread_pool& pool,
                       std::string content_dir = "content")
      : content_dir_(std::move(content_dir)),
        documents_strand_(pool.get_executor()),
        pool_(pool) {
    boost::filesystem::path content_path = content_dir_;
    VERIFY(boost::filesystem::exists(content_path),
           fmt::format("content path does not exist: {}", content_dir));
    RestoreFiles();
  }

  ~FileManager() {
    pool_.stop();
    pool_.join();
  }

  cti::continuable<bool> StoreOrUpdateFile(std::string filename,
                                           std::string content,
                                           uint64_t max_age) {
    return UpdateContentOrCreateDocument(filename, std::move(content), max_age)
        .then([this, f = std::move(filename)](bool result) {
          return cti::make_continuable<bool>(
              [this, f = std::move(f), result](auto&& promise) {
                std::experimental::post(
                    documents_strand_, [this, p = std::move(promise),
                                        f = std::move(f), result]() mutable {
                      auto it = document_by_name_.find(f);
                      VERIFY(it != document_by_name_.end(),
                             fmt::format("no document with name {} found", f));
                      std::experimental::post(
                          pool_, [this, p = std::move(p), f = std::move(f), it,
                                  result]() mutable {
                            DumpOnDisk(fmt::format("{0}/{1}", content_dir_, f),
                                       *it->second);
                            p.set_value(result);
                          });
                    });
              });
        });
  }

  auto RemoveFile(std::string filename) {
    // bool is unnecessary here but it doesn't compile with void.
    return cti::make_continuable<bool>(
        [this, f = std::move(filename)](auto&& promise) {
          std::experimental::post(pool_, [this, p = std::move(promise),
                                          f = std::move(f)]() mutable {
            RemoveFileFromDisk(f);

            std::experimental::post(
                documents_strand_,
                [this, p = std::move(p), f = std::move(f)]() mutable {
                  p.set_value(RemoveFileFromMap(std::move(f)));
                });
          });
        });
  }

  auto RemoveOutdatedFiles() {
    return cti::make_continuable<void>([this](auto&& promise) {
      std::experimental::post(
          documents_strand_, [this, p = std::move(promise)]() mutable {
            auto now = NowCount();

            std::vector<std::string> remove_from_disk;
            for (auto it = documents_with_deadline_.begin();
                 it != documents_with_deadline_.end(); ++it) {
              if (it->first > now) {
                break;
              }
              remove_from_disk.push_back(it->second->FileName);
              RemoveFileFromMap(it->second->FileName);
            }

            std::experimental::post(
                pool_, [this, p = std::move(p),
                        remove = std::move(remove_from_disk)]() mutable {
                  for (auto&& filename : remove) {
                    RemoveFileFromDisk(filename);
                  }
                  p.set_value();
                });
          });
    });
  }

  bool IsFileStillAlive(std::string filename) {
    return cti::make_continuable<bool>(
               [this, f = std::move(filename)](auto&& promise) {
                 std::experimental::post(
                     documents_strand_, [this, p = std::move(promise),
                                         f = std::move(f)]() mutable {
                       auto now = NowCount();
                       auto it = document_by_name_.find(f);
                       VERIFY(it != document_by_name_.end(),
                              fmt::format(
                                  "no file found for aliveness check: {0}", f));
                       LOG(INFO) << "file still alive: " << f;
                       p.set_value(it->second->ExpirationTime() > now);
                     });
               })
        .apply(cti::transforms::wait());
  }

  auto GetDocuments() {
    return cti::make_continuable<std::vector<ParsedDoc>>(
        [this](auto&& promise) {
          std::experimental::post(
              documents_strand_, [this, p = std::move(promise)]() mutable {
                std::vector<ParsedDoc> documents;
                documents.reserve(document_by_name_.size());
                for (auto& [name, document_ptr] : document_by_name_) {
                  documents.push_back(*document_ptr);
                }

                p.set_value(std::move(documents));
              });
        });
  }

  cti::continuable<std::vector<ParsedDoc>> FetchChangeLog() {
    return cti::make_continuable<std::vector<ParsedDoc>>(
        [this](auto&& promise) {
          std::experimental::post(documents_strand_,
                                  [this, p = std::move(promise)]() mutable {
                                    std::vector<ParsedDoc> documents;
                                    change_log_.swap(documents);
                                    p.set_value(std::move(documents));
                                  });
        });
  }

 private:
  auto CreateDocument(std::string filename, std::string content,
                      uint64_t max_age) {
    return cti::make_continuable<void>([this, f = std::move(filename),
                                        c = std::move(content),
                                        age = max_age](auto&& promise) {
      std::experimental::post(
          documents_strand_, [this, p = std::move(promise), f = std::move(f),
                              c = std::move(c), age = age]() mutable {
            auto document_ptr = std::make_unique<ParsedDoc>(
                std::move(f), std::move(c), age, ParsedDoc::EState::Added);

            auto address = document_ptr.get();

            LOG(INFO) << "create document: " << document_ptr->FileName;

            change_log_.push_back(*document_ptr);

            documents_with_deadline_.emplace(document_ptr->ExpirationTime(),
                                             address);
            document_by_name_.emplace(document_ptr->FileName,
                                      std::move(document_ptr));

            p.set_value();
          });
    });
  }

  void DumpOnDisk(std::string_view filepath, const ParsedDoc& document) {
    LOG(INFO) << "write json to file: " << filepath;

    boost::filesystem::ofstream file(filepath.data());
    file << document.Serialize();
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
      nlohmann::json value;
      try {
        file >> value;
      } catch (...) {
        RemoveFileFromDisk(entry.path());
        LOG(INFO) << "file contains invalid json " << entry.path();
        continue;
      }
      auto deadline = value["deadline"].get<uint64_t>();
      auto content = value["content"].get<std::string>();

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

  // Make sure to call it from strand.
  bool RemoveFileFromMap(std::string filename) {
    auto it = document_by_name_.find(filename);
    if (it == document_by_name_.end()) {
      return false;
    }

    it->second->State = ParsedDoc::EState::Removed;

    change_log_.emplace_back(it->second->FileName, std::move(it->second->Data),
                             it->second->MaxAge, it->second->State);

    documents_with_deadline_.erase(
        {it->second->ExpirationTime(), it->second.get()});
    document_by_name_.erase(it);
    return true;
  }

  cti::continuable<bool> UpdateContentOrCreateDocument(std::string filename,
                                                       std::string content,
                                                       uint64_t max_age) {
    return cti::make_continuable<bool>([this, f = std::move(filename),
                                        c = std::move(content), a = max_age](
                                           cti::promise<bool>&& promise) {
      std::experimental::post(
          documents_strand_, [this, p = std::move(promise), f = std::move(f),
                              c = std::move(c), a = a]() mutable {
            auto it = document_by_name_.find(f);
            if (it != document_by_name_.end()) {
              auto* document = it->second.get();

              documents_with_deadline_.erase(
                  {document->ExpirationTime(), document});

              *document = ParsedDoc(document->FileName, std::move(c), a,
                                    ParsedDoc::EState::Changed);

              change_log_.push_back(*document);

              documents_with_deadline_.emplace(document->ExpirationTime(),
                                               document);

              p.set_value(true);
              return;
            }
            CreateDocument(std::move(f), std::move(c), a)
                .then([p = std::move(p)]() mutable { p.set_value(false); });
          });
    });
  }

  void RemoveFileFromDisk(boost::filesystem::path filepath) {
    LOG(INFO) << fmt::format("removing file from disk {0}: {1}",
                             filepath.string(),
                             boost::filesystem::remove(filepath));
  }

 private:
  std::string content_dir_;
  std::vector<ParsedDoc> change_log_;
  std::unordered_map<std::string, std::unique_ptr<ParsedDoc>> document_by_name_;
  std::set<std::pair<uint64_t, ParsedDoc*>> documents_with_deadline_;
  std::experimental::strand<std::experimental::thread_pool::executor_type>
      documents_strand_;
  std::experimental::thread_pool& pool_;
};

}  // namespace tgnews

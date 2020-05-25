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
#include "base/context.h"
#include "fmt/format.h"
#include "glog/logging.h"
#include "third_party/nlohmann_json/single_include/nlohmann/json.hpp"

namespace tgnews {

class FileManager {
 public:
  explicit FileManager(std::experimental::thread_pool& pool, Context* context,
                       std::string content_dir = "content");

  ~FileManager();

  cti::continuable<bool> StoreOrUpdateFile(std::string filename,
                                           std::string content,
                                           uint64_t max_age);

  cti::continuable<bool> RemoveFile(std::string filename);

  cti::continuable<bool> RemoveOutdatedFiles();

  bool IsFileStillAlive(std::string filename);

  cti::continuable<std::vector<ParsedDoc>> GetDocuments();

  cti::continuable<std::vector<ParsedDoc>> FetchChangeLog();

 private:
  cti::continuable<bool> EmplaceDocument(std::unique_ptr<ParsedDoc> document);

  void DumpOnDisk(std::string_view filepath, const ParsedDoc& document);

  void RestoreFiles();

  // Make sure to call it from strand.
  bool RemoveFileFromMap(std::string filename);

  cti::continuable<bool> UpdateContentOrCreateDocument(std::string filename,
                                                       std::string content,
                                                       uint64_t max_age);

  cti::continuable<ParsedDoc> CreateDocument(std::string filename,
                                             std::string content,
                                             uint64_t max_age,
                                             ParsedDoc::EState state);

  void RemoveFileFromDisk(boost::filesystem::path filepath);

 private:
  Context* context_;
  std::string content_dir_;
  std::atomic<uint64_t> last_fetch_time_ = 0;
  std::vector<ParsedDoc> change_log_;
  std::unordered_map<std::string, std::unique_ptr<ParsedDoc>> document_by_name_;
  std::set<std::pair<uint64_t, ParsedDoc*>> documents_with_deadline_;
  std::experimental::strand<std::experimental::thread_pool::executor_type>
      documents_strand_;
  std::experimental::thread_pool& pool_;
};

}  // namespace tgnews

#include "base/file_manager.h"

namespace tgnews {

FileManager::FileManager(std::experimental::thread_pool& pool, Context* context,
                         std::string content_dir)
    : content_dir_(std::move(content_dir)),
      context_(context),
      documents_strand_(pool.get_executor()),
      pool_(pool) {
  boost::filesystem::path content_path = content_dir_;
  if (!boost::filesystem::exists(content_path)) {
    LOG(INFO) << fmt::format("content path does not exist: {}", content_dir);
    VERIFY(boost::filesystem::create_directory(content_path),
           fmt::format("unable to create directory for index storage: {}",
                       content_dir));
  }

  std::experimental::post(documents_strand_, [this] {
    RestoreFiles();
    finished_restoring_from_disk_ = true;
  });
}

FileManager::~FileManager() {
  pool_.stop();
  pool_.join();
}

cti::continuable<bool> FileManager::StoreOrUpdateFile(std::string filename,
                                                      std::string content,
                                                      uint64_t max_age) {
  return UpdateContentOrCreateDocument(filename, std::move(content), max_age)
      .then([this, f = std::move(filename)](bool result) mutable {
        LOG(INFO) << "after content creation";
        return cti::make_continuable<bool>(
            [this, f = std::move(f), result](auto promise) mutable {
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

cti::continuable<bool> FileManager::RemoveFile(std::string filename) {
  // bool is unnecessary here but it doesn't compile with void.
  return cti::make_continuable<bool>(
      [this, f = std::move(filename)](auto promise) mutable {
        std::experimental::post(
            pool_, [this, p = std::move(promise), f = std::move(f)]() mutable {
              RemoveFileFromDisk(f);

              std::experimental::post(
                  documents_strand_,
                  [this, p = std::move(p), f = std::move(f)]() mutable {
                    p.set_value(RemoveFileFromMap(std::move(f)));
                  });
            });
      });
}

cti::continuable<bool> FileManager::RemoveOutdatedFiles() {
  if (!finished_restoring_from_disk_) {
    return cti::make_ready_continuable<bool>(true);
  }
  return cti::make_continuable<bool>([this](auto promise) {
    std::experimental::post(
        documents_strand_, [this, p = std::move(promise)]() mutable {
          auto now = last_fetch_time_.load();

          std::vector<std::string> remove_from_disk;
          for (auto it = documents_with_deadline_.begin();
               it != documents_with_deadline_.end(); ++it) {
            if (it->first > now) {
              break;
            }
            LOG(INFO) << fmt::format(
                "now: {} remove outdated file: {} with deadline: {}", now,
                it->second->FileName, it->first);
            remove_from_disk.push_back(it->second->FileName);
            RemoveFileFromMap(it->second->FileName);
          }

          std::experimental::post(
              pool_, [this, p = std::move(p),
                      remove = std::move(remove_from_disk)]() mutable {
                for (auto&& filename : remove) {
                  RemoveFileFromDisk(filename);
                }
                p.set_value(true);
              });
        });
  });
}

cti::continuable<std::vector<ParsedDoc>> FileManager::GetDocuments() {
  return cti::make_continuable<std::vector<ParsedDoc>>([this](auto promise) {
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

cti::continuable<std::vector<ParsedDoc>> FileManager::FetchChangeLog() {
  return cti::make_continuable<std::vector<ParsedDoc>>([this](auto promise) {
    std::experimental::post(documents_strand_,
                            [this, p = std::move(promise)]() mutable {
                              std::vector<ParsedDoc> documents;
                              change_log_.swap(documents);
                              p.set_value(std::move(documents));
                            });
  });
}

cti::continuable<bool> FileManager::EmplaceDocument(
    std::unique_ptr<ParsedDoc> document) {
  LOG(INFO) << fmt::format("start emplacing document: {}", document->FileName);
  return cti::make_continuable<bool>(
      [this, d = std::move(document)](auto promise) mutable {
        LOG(INFO) << "in future";
        LOG(INFO) << fmt::format("start emplacing document in future: {}",
                                 d->FileName);
        std::experimental::post(
            documents_strand_,
            [this, p = std::move(promise), document = std::move(d)]() mutable {
              EmplaceDocumentSync(std::move(document));

              p.set_value(true);
            });
      });
}

void FileManager::EmplaceDocumentSync(std::unique_ptr<ParsedDoc> document) {
  LOG(INFO) << fmt::format("create document: {} fetch time: {}",
                           document->FileName, document->FetchTime);

  last_fetch_time_ = std::max(last_fetch_time_.load(), document->FetchTime);

  change_log_.push_back(*document);

  documents_with_deadline_.emplace(document->ExpirationTime(), document.get());
  document_by_name_.emplace(document->FileName, std::move(document));
}

void FileManager::DumpOnDisk(std::string_view filepath,
                             const ParsedDoc& document) {
  LOG(INFO) << "write json to file: " << filepath;

  boost::filesystem::ofstream file(filepath.data());
  file << document.Serialize();
  file.close();

  boost::filesystem::path path = filepath.data();
  VERIFY(boost::filesystem::exists(path),
         "file has to be on disk after it's written");

  LOG(INFO) << "written on disk: " << filepath;
}

void FileManager::RestoreFiles() {
  boost::filesystem::path path = content_dir_;
  for (const auto& entry : boost::make_iterator_range(
           boost::filesystem::directory_iterator(path), {})) {
    boost::filesystem::ifstream file(entry.path());
    nlohmann::json value;
    std::unique_ptr<ParsedDoc> document;
    try {
      file >> value;
      document = std::make_unique<ParsedDoc>(std::move(value));
    } catch (...) {
      LOG(INFO) << "file contains invalid json: " << entry.path();
      RemoveFileFromDisk(entry.path());
      continue;
    }

    EmplaceDocumentSync(std::move(document));
  }

  LOG(INFO) << "restored file count: " << document_by_name_.size();
}

// Make sure to call it from strand.
bool FileManager::RemoveFileFromMap(std::string filename) {
  auto it = document_by_name_.find(filename);
  if (it == document_by_name_.end()) {
    return false;
  }

  it->second->State = ParsedDoc::EState::Removed;

  auto expiration_time = it->second->ExpirationTime();
  auto* address = it->second.get();

  auto document = std::move(*it->second);

  documents_with_deadline_.erase({expiration_time, address});
  document_by_name_.erase(it);

  change_log_.emplace_back(std::move(document));

  return true;
}

cti::continuable<bool> FileManager::UpdateContentOrCreateDocument(
    std::string filename, std::string content, uint64_t max_age) {
  return cti::make_continuable<bool>([this, f = std::move(filename),
                                      c = std::move(content), a = max_age](
                                         cti::promise<bool> promise) mutable {
    std::experimental::post(documents_strand_, [this, p = std::move(promise),
                                                f = std::move(f),
                                                c = std::move(c),
                                                a = a]() mutable {
      auto it = document_by_name_.find(f);
      if (it != document_by_name_.end()) {
        auto* document = it->second.get();

        auto new_document =
            ParsedDoc(context_, document->FileName, std::move(c), a,
                      ParsedDoc::EState::Changed);

        documents_with_deadline_.erase({document->ExpirationTime(), document});

        *document = std::move(new_document);

        last_fetch_time_ =
            std::max(last_fetch_time_.load(), document->FetchTime);

        change_log_.push_back(*document);

        documents_with_deadline_.emplace(document->ExpirationTime(), document);

        p.set_value(true);
        return;
      }
      auto new_document = ParsedDoc(context_, std::move(f), std::move(c), a,
                                    ParsedDoc::EState::Added);
      auto document_ptr = std::make_unique<ParsedDoc>(std::move(new_document));
      EmplaceDocumentSync(std::move(document_ptr));
      p.set_value(false);
    });
  });
}

cti::continuable<ParsedDoc> FileManager::CreateDocument(
    std::string filename, std::string content, uint64_t max_age,
    ParsedDoc::EState state) {
  return cti::make_continuable<ParsedDoc>(
      [this, filename = std::move(filename), content = std::move(content),
       max_age = max_age, state = state](auto promise) mutable {
        std::experimental::post(
            pool_,
            [this, p = std::move(promise), filename = std::move(filename),
             content = std::move(content), max_age = max_age,
             state = state]() mutable {
              p.set_value(ParsedDoc(context_, std::move(filename),
                                    std::move(content), max_age, state));
            });
      });
}

void FileManager::RemoveFileFromDisk(boost::filesystem::path filepath) {
  LOG(INFO) << fmt::format("removing file from disk {0}: {1}",
                           filepath.string(),
                           boost::filesystem::remove(filepath));
}

}  // namespace tgnews

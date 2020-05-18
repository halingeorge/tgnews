#pragma once
#include "base/file_cache.h"

#include <string>
#include <memory>

#include "third_party/fastText/src/fasttext.h"
#include "third_party/onmt_tokenizer/include/onmt/Tokenizer.h"

namespace tgnews {

class Context {
 public:
  Context(const std::string modelPath, std::unique_ptr<FileCache> fileCache);
  std::unique_ptr<FileCache> fileCache;
  std::unique_ptr<fasttext::FastText> LangDetect;
  std::unique_ptr<fasttext::FastText> RuCatModel;
  std::unique_ptr<fasttext::FastText> EnCatModel;
  onmt::Tokenizer Tokenizer;
};

}

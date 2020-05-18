#pragma once
#include "base/file_cache.h"

#include <string>
#include <memory>

#include "third_party/fastText/src/fasttext.h"
//#include "onmt/Tokenizer.h"

namespace tgnews {

class Context {
 public:
  Context(const std::string modelPath, FileCache fileCache);
  std::unique_ptr<fasttext::FastText> langDetect;
  FileCache fileCache;
  //onmt::Tokenizer Tokenizer;
};

}

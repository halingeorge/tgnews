#pragma once 
#include <string>
#include <memory>

#include "third_party/fastText/src/fasttext.h"

namespace tgnews {

  class Context {
  public:
    Context(const std::string modelPath);
    std::unique_ptr<fasttext::FastText> langDetect;
  };

}
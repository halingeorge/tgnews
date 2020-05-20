#pragma once
#include "base/file_cache.h"

#include <string>
#include <memory>

#include "third_party/fastText/src/fasttext.h"
#include "third_party/onmt_tokenizer/include/onmt/Tokenizer.h"
#include "third_party/eigen/Eigen/Core"

namespace tgnews {

class Context {
 public:
  Context(const std::string modelPath, std::unique_ptr<FileCache> fileCache);
  std::unique_ptr<FileCache> fileCache;
  std::unique_ptr<fasttext::FastText> LangDetect;
  std::unique_ptr<fasttext::FastText> RuCatModel;
  std::unique_ptr<fasttext::FastText> EnCatModel;
  std::unique_ptr<fasttext::FastText> RuVecModel;
  std::unique_ptr<fasttext::FastText> EnVecModel;
  Eigen::MatrixXf RuMatrix;
  Eigen::VectorXf RuBias;
  Eigen::MatrixXf EnMatrix;
  Eigen::VectorXf EnBias;
  onmt::Tokenizer Tokenizer;
};

}

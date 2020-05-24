#include "base/context.h"

#include "glog/logging.h"

#include <fstream>

namespace {

void LoadMatrixFromFile(Eigen::MatrixXf& matrix, std::string file) {
  std::ifstream matrixIn(file);
  std::string line;
  int row = 0;
  int col = 0;
  while (std::getline(matrixIn, line)) {
    std::string num;
    for (char ch : line) {
      if (ch == ' ') {
        continue;
        }
      if (ch != ',' && ch != '\n') {
        num += ch;
        continue;
      }
      matrix(col++, row) = std::stof(num);
      num = "";
    }
    if (!num.empty()) {
      matrix(col, row) = std::stof(num);
      num = "";
    }
    col = 0;
    row++;
  }
  matrixIn.close();
  assert(row != 0);
  assert(row == 50);
}

void LoadBiasFromFile(Eigen::VectorXf& vec, std::string file) {
  std::ifstream biasIn(file);
  int row = 0;
  std::string line;
  while (std::getline(biasIn, line)) {
    vec(row++) = std::stof(line);
  }
  biasIn.close();
}

}

namespace tgnews {

Context::Context(const std::string modelPath, std::unique_ptr<FileCache> fileCache)
  : fileCache(std::move(fileCache))
  , Tokenizer(onmt::Tokenizer::Mode::Conservative, onmt::Tokenizer::Flags::CaseFeature)
  , Ratings(modelPath + "/pagerank_rating.txt")
{
  LOG(INFO) << "context ctr";
  LOG(INFO) << "loading lang detect model";
  LangDetect = std::make_unique<fasttext::FastText>();
  LangDetect->loadModel(modelPath + "/lang_detect.ftz");
  LOG(INFO) << "loaded";
  LOG(INFO) << "load ru cat model";
  RuCatModel = std::make_unique<fasttext::FastText>();
  RuCatModel->loadModel(modelPath + "/ru_cat_v2.ftz");
  EnCatModel = std::make_unique<fasttext::FastText>();
  EnCatModel->loadModel(modelPath + "/en_cat_v2.ftz");
  RuVecModel = std::make_unique<fasttext::FastText>();
  RuVecModel->loadModel(modelPath + "/ru_vectors_v2.bin");
  EnVecModel = std::make_unique<fasttext::FastText>();
  EnVecModel->loadModel(modelPath + "/en_vectors_v2.bin");

  RuMatrix = Eigen::MatrixXf(RuVecModel->getDimension() * 3, 50); 
  LoadMatrixFromFile(RuMatrix, modelPath + "/ru_sentence_embedder/matrix.txt");

  EnMatrix = Eigen::MatrixXf(EnVecModel->getDimension() * 3, 50);
  LoadMatrixFromFile(EnMatrix, modelPath + "/en_sentence_embedder/matrix.txt");
  
  RuBias = Eigen::VectorXf(50);
  LoadBiasFromFile(RuBias, modelPath + "/ru_sentence_embedder/bias.txt");

  EnBias = Eigen::VectorXf(50);
  LoadBiasFromFile(EnBias, modelPath + "/en_sentence_embedder/bias.txt");
}

}

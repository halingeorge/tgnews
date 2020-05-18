#include "base/context.h"
namespace tgnews {

Context::Context(const std::string modelPath, FileCache fileCache) 
  : fileCache(std::move(fileCache))
  , Tokenizer(onmt::Tokenizer::Mode::Conservative, onmt::Tokenizer::Flags::CaseFeature)
{
  LangDetect = std::make_unique<fasttext::FastText>();
  LangDetect->loadModel(modelPath + "/lang_detect.ftz");
  RuCatModel = std::make_unique<fasttext::FastText>();
  RuCatModel->loadModel(modelPath + "/ru_cat_v2.ftz");
  EnCatModel = std::make_unique<fasttext::FastText>();
  EnCatModel->loadModel(modelPath + "/en_cat_v2.ftz");
}

}

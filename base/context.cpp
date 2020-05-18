#include "base/context.h"
namespace tgnews {

Context::Context(const std::string modelPath) 
  //: Tokenizer(onmt::Tokenizer::Mode::Conservative, onmt::Tokenizer::Flags::CaseFeature) 
{
  langDetect = std::make_unique<fasttext::FastText>();
  langDetect->loadModel(modelPath + "/lang_detect.ftz");
}

}

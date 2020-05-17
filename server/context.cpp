#include "server/context.h"
namespace tgnews {
  
  Context::Context(const std::string modelPath) {
    langDetect = std::make_unique<fasttext::FastText>();
    langDetect->loadModel(modelPath + "/lang_detect.ftz");
  }

}

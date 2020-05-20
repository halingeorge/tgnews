#include "solver/embedder.h"

namespace tgnews {
  fasttext::Vector Embedder::GetEmbedding(const tgnews::ParsedDoc& document) const {
    std::istringstream ss(document.GoodTitle + " " + document.GoodText);
    const size_t N = Model->getDimension();
    fasttext::Vector wordVector(N);
    fasttext::Vector avgVector(N);
    fasttext::Vector maxVector(N);
    fasttext::Vector minVector(N);
    std::string word;
    size_t count = 0;
    while (ss >> word) {
        if (count > 100) {
            break;
        }
        Model->getWordVector(wordVector, word);
        float norm = wordVector.norm();
        if (norm < 0.0001f) {
            continue;
        }
        wordVector.mul(1.0f / norm);

        avgVector.addVector(wordVector);
        if (count == 0) {
            maxVector = wordVector;
            minVector = wordVector;
        } else {
            for (size_t i = 0; i < N; i++) {
                maxVector[i] = std::max(maxVector[i], wordVector[i]);
                minVector[i] = std::min(minVector[i], wordVector[i]);
            }
        }
        count += 1;
    }
    if (count > 0) {
        avgVector.mul(1.0f / static_cast<float>(count));
    }
    fasttext::Vector resultVector(N);
    Eigen::VectorXf concatVector(3 * N);
    for (size_t i = 0; i < N; i++) {
        concatVector[i] = avgVector[i];
        concatVector[N + i] = maxVector[i];
        concatVector[2 * N + i] = minVector[i];
    }
    auto eigenResult = ((Matrix.transpose() * concatVector) + Bias).transpose();
    for (size_t i = 0; i < N; i++) {
        resultVector[i] = eigenResult(0, i);
    }
    return resultVector;
  }
}

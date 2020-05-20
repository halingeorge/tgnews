#include "solver/parsed_document.h"

#include "third_party/fastText/src/fasttext.h"
#include "third_party/eigen/Eigen/Core"

namespace tgnews {
  class Embedder {
  public:
    Embedder(fasttext::FastText* model, const Eigen::MatrixXf& matrix, const Eigen::VectorXf& bias)
      : Model(model)
      , Matrix(matrix)
      , Bias(bias)
    {}

    fasttext::Vector GetEmbedding(const tgnews::ParsedDoc& document) const;
  private:
    fasttext::FastText* Model;
    const Eigen::MatrixXf& Matrix;
    const Eigen::VectorXf& Bias;
  };
}

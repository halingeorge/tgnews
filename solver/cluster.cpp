#include "cluster.h"

namespace {
  using namespace tgnews;

  void FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) {
    distances = -((points * points.transpose()).array() + 1.0f) / 2.0f + 1.0f;
    distances += distances.Identity(distances.rows(), distances.cols());
  }

  std::vector<size_t> RunClusteringImpl(Eigen::MatrixXf& points, float DistanceThreshold) {
    size_t docSize = points.rows();
    Eigen::MatrixXf distances(points.rows(), points.rows());
    FillDistanceMatrix(points, distances);
    const float INF_DISTANCE = 1.0f;

    // Prepare 3 arrays
    std::vector<size_t> labels(docSize);
    for (size_t i = 0; i < docSize; i++) {
        labels[i] = i;
    }
    std::vector<size_t> nn(docSize);
    std::vector<float> nnDistances(docSize);
    for (size_t i = 0; i < docSize; i++) {
        Eigen::Index minJ;
        nnDistances[i] = distances.row(i).minCoeff(&minJ);
        nn[i] = minJ;
    }

    // Main linking loop
    size_t level = 0;
    while (level + 1 < docSize) {
      // Calculate minimal distance
      auto minDistanceIt = std::min_element(nnDistances.begin(), nnDistances.end());
      size_t minI = std::distance(nnDistances.begin(), minDistanceIt);
      size_t minJ = nn[minI];
      float minDistance = *minDistanceIt;
      if (minDistance > DistanceThreshold) {
        break;
      }

      // Link minJ to minI
      for (size_t i = 0; i < docSize; i++) {
        if (labels[i] == minJ) {
          labels[i] = minI;
        }
      }

      // Update distance matrix and nearest neighbors
      nnDistances[minI] = INF_DISTANCE;
      for (size_t k = 0; k < static_cast<size_t>(distances.rows()); k++) {
        if (k == minI || k == minJ) {
          continue;
        }
        float newDistance = std::min(distances(minJ, k), distances(minI, k));
        distances(minI, k) = newDistance;
        distances(k, minI) = newDistance;
        if (newDistance < nnDistances[minI]) {
          nnDistances[minI] = newDistance;
          nn[minI] = k;
        }
      }

      // Remove minJ row and column from distance matrix
      nnDistances[minJ] = INF_DISTANCE;
      for (size_t i = 0; i < static_cast<size_t>(distances.rows()); i++) {
        distances(minJ, i) = INF_DISTANCE;
        distances(i, minJ) = INF_DISTANCE;
      }
      level += 1;
    }

    return labels;
  }

  std::vector<size_t> GetLabels(std::vector<ParsedDoc>& docs, float threshold) {
    const size_t docSize = docs.size();;
    const size_t embSize = 50; // sorry, mario
    Eigen::MatrixXf points(docSize, embSize);
    for (size_t i = 0; i < docSize; ++i) {
        fasttext::Vector embedding = docs[i].Vector;
        Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
        points.row(i) = eigenVector / eigenVector.norm();
    }
    
    return RunClusteringImpl(points, threshold);
  }

}


namespace tgnews {

  std::vector<Cluster> RunClustering(std::vector<ParsedDoc>& docs) {
    std::vector<ParsedDoc> ruDocs, enDocs;
    for (auto& doc : docs) {
      if (doc.Lang && doc.Lang == "ru" && doc.IsNews()) {
        ruDocs.push_back(doc);
      } else if (doc.Lang && doc.Lang == "en" && doc.IsNews()) {
        enDocs.push_back(doc);
      }
    }
    auto ruLabels = GetLabels(ruDocs, 0.013f);
    auto enLabels = GetLabels(enDocs, 0.02f);
    std::vector<Cluster> ruCluster(ruDocs.size()), enCluster(enDocs.size());
    for (size_t idx = 0; idx < ruDocs.size(); ++idx) {
      ruCluster[ruLabels[idx]].AddDocument(ruDocs[idx]);
    }
    for (size_t idx = 0; idx < enDocs.size(); ++idx) {
      enCluster[enLabels[idx]].AddDocument(enDocs[idx]);
    }
    std::vector<Cluster> result;
    for (auto& c : ruCluster) {
      if (c.Size() > 0) {
        c.Init();
        result.push_back(c);
      }
    }
    for (auto& c : enCluster) {
      if (c.Size() > 0) {
        c.Init();
        result.push_back(c);
      }
    }
    for (auto& c : result) {
      c.Sort();
    }
    // sorted afterwards
    std::sort(result.begin(), result.end(), [](const auto& l, const auto& r) {return l.GetTime() > r.GetTime();});
    return result;
  }

}

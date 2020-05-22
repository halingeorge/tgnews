#pragma once

#include "solver/parsed_document.h"

namespace tgnews {

  class Cluster {
  public:
    void AddDocument(const ParsedDoc& doc) {
      Docs.push_back(doc);
    }
    std::string GetTitle() const {
      return Docs[0].Title;
    }
    size_t Size() const {
      return Docs.size();
    }
    const std::vector<ParsedDoc>& GetDocs() const {
      return Docs;
    }

    ENewsCategory GetCategory() const {
      std::vector<size_t> categoryCount(NC_COUNT);
      for (const auto& doc : Docs) {
        ENewsCategory docCategory = doc.Category;
        assert(docCategory != NC_UNDEFINED && docCategory != NC_NOT_NEWS);
        categoryCount[static_cast<size_t>(docCategory)] += 1;
      }
      auto it = std::max_element(categoryCount.begin(), categoryCount.end());
      return static_cast<ENewsCategory>(std::distance(categoryCount.begin(), it));
    }
    void Sort() {
      sort(Docs.begin(), Docs.end(), [](const auto& l, const auto& r) {return l.Weight > r.Weight;});
    }
    float Weight() const {
      float sum = 0.f;
      for (const auto& d : Docs) {
        sum += d.Weight;
      }
      return sum;
    }
  private: 
    std::vector<ParsedDoc> Docs;
  };

  std::vector<Cluster> RunClustering(std::vector<ParsedDoc>& docs);

}

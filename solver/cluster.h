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
  private: 
    std::vector<ParsedDoc> Docs;
  };

  std::vector<Cluster> RunClustering(std::vector<ParsedDoc>& docs);

}

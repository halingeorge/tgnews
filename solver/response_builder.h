#pragma once
#include "base/parsed_document.h"
#include "cluster.h"

#include "base/context.h"

#include <memory>

namespace tgnews {

const std::vector<int64_t> Discretization = {300, 1800, 3600, 14400, 28800, 86400, 172800, 345600, 604800, 1209600, 1814400, 2592000};
constexpr size_t DiscretizationSize = 12;

class CalculatedResponses {
 public:
  CalculatedResponses(const std::string& path);
  CalculatedResponses(const std::vector<tgnews::ParsedDoc>& docs, const std::vector<Cluster>& clustering);
  nlohmann::json GetAns(const std::string& lang = {}, const std::string& category = {}, const uint64_t period = 0);
 public:
  void dump(const std::string& path);
  nlohmann::json LangAns;
  nlohmann::json NewsAns;
  nlohmann::json CategoryAns;
  nlohmann::json ThreadsAns;
  std::array<std::array<std::array<nlohmann::json, LangCount>, ENewsCategory::NC_COUNT>, DiscretizationSize> Answers;
  
};

class ResponseBuilder {
 public:
  ResponseBuilder(tgnews::Context* context);
  CalculatedResponses AddDocuments(std::vector<ParsedDoc> docs);

 private:
  std::vector<tgnews::ParsedDoc> Docs;
  tgnews::Context* Context;
};

}

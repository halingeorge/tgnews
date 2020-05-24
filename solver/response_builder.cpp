#include "response_builder.h"
#include "embedder.h"

#include <vector>
#include <string>
#include <chrono>
#include <exception>

namespace {

using namespace tgnews;

nlohmann::json CalcLangAns(const std::vector<tgnews::ParsedDoc>& docs) {
  nlohmann::json result = nlohmann::json::array();
  for (const auto& lang : {"en", "ru"}) {
    nlohmann::json lang_obj;
    lang_obj["lang_code"] = lang;
    nlohmann::json articles = nlohmann::json::array();
    for (const auto& doc : docs) {
      if (doc.Lang == lang) {
        articles.push_back(doc.FileName);
      }
    }
    lang_obj["articles"] = articles;
    result.push_back(lang_obj);
  }
  return result;
}

nlohmann::json CalcNewsAns(const std::vector<tgnews::ParsedDoc>& docs) {
  nlohmann::json articles = nlohmann::json::array();  
  for (const auto& doc : docs) {
    if (doc.IsNews()) {
      articles.push_back(doc.FileName);
    }
  }
  nlohmann::json result;
  result["articles"] = std::move(articles);
  return result;
}

nlohmann::json CalcCategoryAns(const std::vector<tgnews::ParsedDoc>& docs) {
  nlohmann::json society = nlohmann::json::array();
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_SOCIETY) {
      society.push_back(doc.FileName);
    }
  }
  nlohmann::json economy = nlohmann::json::array();
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_ECONOMY) {
      economy.push_back(doc.FileName);
    }
  }
  nlohmann::json technology = nlohmann::json::array();
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_TECHNOLOGY) {
      technology.push_back(doc.FileName);
    }
  }
  nlohmann::json sports = nlohmann::json::array();
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_SPORTS) {
      sports.push_back(doc.FileName);
    }
  }
  nlohmann::json entertainment = nlohmann::json::array();
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_ENTERTAINMENT) {
      entertainment.push_back(doc.FileName);
    }
  }
  nlohmann::json science = nlohmann::json::array();
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_SCIENCE) {
      science.push_back(doc.FileName);
    }
  }
  nlohmann::json other = nlohmann::json::array();
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_OTHER) {
      other.push_back(doc.FileName);
    }
  }
  nlohmann::json result = nlohmann::json::array();
#define ADD_TO_RESULT(s) { \
                         nlohmann::json tmp; \
                         tmp["category"] = #s; \
                         tmp["articles"] = s; \
                         result.push_back(tmp); \
                         }
  ADD_TO_RESULT(society);
  ADD_TO_RESULT(economy);
  ADD_TO_RESULT(technology);
  ADD_TO_RESULT(sports);
  ADD_TO_RESULT(entertainment);
  ADD_TO_RESULT(science);
  ADD_TO_RESULT(other);
  return result;
}

nlohmann::json CalcThreadsAns(const std::vector<Cluster>& clusters) {
  nlohmann::json threads = nlohmann::json::array();
  std::vector<std::pair<float, size_t>> weights;
  for (size_t idx = 0; idx < clusters.size(); ++idx) {
    const auto& c = clusters[idx];
    weights.push_back({c.Weight(), idx});
  }
  std::sort(weights.begin(), weights.end(), std::greater<std::pair<float, size_t>>());
  for (size_t idx = 0; idx < clusters.size(); ++idx) {
    const auto& c = clusters[weights[idx].second];
    nlohmann::json thread;
    thread["title"] = c.GetTitle();
    nlohmann::json articles = nlohmann::json::array();
    for (const auto& d : c.GetDocs()) {
      articles.push_back(d.FileName);
    }
    thread["articles"] = std::move(articles);
    threads.push_back(thread);
  }
  return threads;
}

}

namespace tgnews {

CalculatedResponses::CalculatedResponses(const std::vector<tgnews::ParsedDoc>& docs, const std::vector<Cluster>& clustering) {
  LangAns = CalcLangAns(docs);
  NewsAns = CalcNewsAns(docs);
  CategoryAns = CalcCategoryAns(docs);
  ThreadsAns = CalcThreadsAns(clustering);
  uint64_t now = 0;
  if (clustering.size()) {
    now = std::max_element(clustering.begin(), clustering.end(), [](const auto& l, const auto& r) {return l.GetTime() < r.GetTime();})->GetTime();
  }
  size_t durIdx = 0;
  for (const auto& duration : Discretization) {
    using WeightWithIdx = std::pair<float, size_t>;
    std::array<std::array<std::vector<WeightWithIdx>, ELang::LangCount>, ENewsCategory::NC_COUNT> weights;
    for (size_t clusterIdx = 0; clusterIdx < clustering.size(); ++clusterIdx) {
      const auto& c = clustering[clusterIdx];
      if (c.GetTime() + duration < now) {
        break; // cause clusters sorted (by freshness)
      }
      size_t catIdx = static_cast<size_t>(c.GetCategory());
      size_t langIdx = static_cast<size_t>(c.GetEnumLang());
      weights[catIdx][langIdx].push_back({c.Weight(), clusterIdx});
      weights[static_cast<size_t>(NC_ANY)][langIdx].push_back({c.Weight(), clusterIdx});
    }
    for (size_t catIdx = 0; catIdx < NC_COUNT; catIdx++) {
      for (size_t langIdx = 0; langIdx < LangCount; ++langIdx) {
        auto& vec = weights[catIdx][langIdx];
        std::sort(vec.begin(), vec.end(), std::greater<WeightWithIdx>());
        nlohmann::json ans = nlohmann::json::array();
        for (const auto& it : vec) {
          nlohmann::json thread;
          thread["title"] = clustering[it.second].GetTitle();
          nlohmann::json articles = nlohmann::json::array();
          for (const auto& d : clustering[it.second].GetDocs()) {
            articles.push_back(d.FileName);
          }
          thread["articles"] = std::move(articles);
          ans.push_back(thread);
        }
        Answers[durIdx][catIdx][langIdx] = ans;
      }
    }
  }
}

nlohmann::json CalculatedResponses::GetAns(const std::string& lang, const std::string& category, const uint64_t period) {
  size_t idx = std::distance(Discretization.begin(), std::upper_bound(Discretization.begin(), Discretization.end(), period));
  if (idx == Discretization.size()) {
    --idx;
  }
  size_t catIdx = NC_COUNT;
  if (category == "any") catIdx = NC_ANY;
  if (category == "society") catIdx = NC_SOCIETY;
  if (category == "economy") catIdx = NC_ECONOMY;
  if (category == "technology") catIdx = NC_TECHNOLOGY;
  if (category == "sports") catIdx = NC_SPORTS;
  if (category == "entertainment") catIdx = NC_ENTERTAINMENT;
  if (category == "science") catIdx = NC_SCIENCE;
  if (category == "other") catIdx = NC_OTHER;
  if (catIdx == NC_COUNT) {
    throw std::runtime_error("unknown cat");
  }
  size_t langIdx = LangCount;
  if (lang == "ru") catIdx = LangRu;
  if (lang == "en") catIdx = LangEn;
  if (langIdx == LangCount) {
    throw std::runtime_error("unknown lang");
  }
  return Answers[idx][catIdx][langIdx];
}

ResponseBuilder::ResponseBuilder(tgnews::Context context) : Context(std::move(context)) {}

CalculatedResponses ResponseBuilder::AddDocuments(const std::vector<Document>& docs) {
  std::cerr << docs.size() << " - docs size" << std::endl;
  for (const auto& doc : docs) {
    Docs.emplace_back(doc);
  }
  {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (auto& doc : Docs) {
      doc.ParseLang(Context.LangDetect.get());
      doc.Tokenize(Context);
      doc.DetectCategory(Context);
      doc.CalcWeight(Context);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cerr << "Time difference categorization = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[milli]" << std::endl;
  }
  auto ruEmbedder = Embedder(Context.RuCatModel.get(), Context.RuMatrix, Context.RuBias);
  auto enEmbedder = Embedder(Context.EnCatModel.get(), Context.EnMatrix, Context.EnBias);
  {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (auto& doc : Docs) {
      if (doc.Lang && doc.Lang == "ru") {
        doc.Vector = ruEmbedder.GetEmbedding(doc);
      } else if (doc.Lang && doc.Lang == "en") {
        doc.Vector = enEmbedder.GetEmbedding(doc);
      }
    }
    std::vector<Cluster> clustering = RunClustering(Docs);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cerr << "Time difference clustering = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[milli]" << std::endl;
    return {Docs, clustering};
  }
}

}

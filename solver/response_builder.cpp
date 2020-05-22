#include "response_builder.h"
#include "embedder.h"

#include <vector>
#include <string>
#include <chrono>
#include <exception>

namespace {

using namespace tgnews;

Json::Value CalcLangAns(const std::vector<tgnews::ParsedDoc>& docs) {
  Json::Value result;
  int idx = 0;
  for (const auto& lang : {"en", "ru"}) {
    result[idx]["lang_code"] = lang;
    int docidx = 0;
    for (const auto& doc : docs) {
      if (doc.Lang == lang) {
        result[idx]["articles"][docidx] = doc.FileName;
        ++docidx;
      }
    }
    ++idx;
  }
  return result;
}

Json::Value CalcNewsAns(const std::vector<tgnews::ParsedDoc>& docs) {
  Json::Value articles;  
  for (const auto& doc : docs) {
    if (doc.IsNews()) {
      articles.append(doc.FileName + *doc.Lang);
    }
  }
  Json::Value result;
  result["articles"] = std::move(articles);
  return result;
}

Json::Value CalcCategoryAns(const std::vector<tgnews::ParsedDoc>& docs) {
  Json::Value society;
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_SOCIETY) {
      society.append(doc.FileName);
    }
  }
  Json::Value economy;
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_ECONOMY) {
      economy.append(doc.FileName);
    }
  }
  Json::Value technology;
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_TECHNOLOGY) {
      technology.append(doc.FileName);
    }
  }
  Json::Value sports;
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_SPORTS) {
      sports.append(doc.FileName);
    }
  }
  Json::Value entertainment;
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_ENTERTAINMENT) {
      entertainment.append(doc.FileName);
    }
  }
  Json::Value science;
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_SCIENCE) {
      science.append(doc.FileName);
    }
  }
  Json::Value other;
  for (const auto& doc : docs) {
    if (doc.Category == ENewsCategory::NC_OTHER) {
      other.append(doc.FileName);
    }
  }
  Json::Value result;
#define ADD_TO_RESULT(s) result[#s] = std::move(s);
  ADD_TO_RESULT(society);
  ADD_TO_RESULT(economy);
  ADD_TO_RESULT(technology);
  ADD_TO_RESULT(sports);
  ADD_TO_RESULT(entertainment);
  ADD_TO_RESULT(science);
  ADD_TO_RESULT(other);
  return result;
}

Json::Value CalcThreadsAns(const std::vector<Cluster>& clusters) {
  Json::Value threads;
  for (const auto& c : clusters) {
    Json::Value thread;
    thread["title"] = c.GetTitle();
    Json::Value articles;
    for (const auto& d : c.GetDocs()) {
      articles.append(d.FileName);
    }
    thread["articles"] = std::move(articles);
    threads.append(thread);
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
  uint64_t now = std::max_element(clustering.begin(), clustering.end(), [](const auto& l, const auto& r) {return l.GetTime() < r.GetTime();})->GetTime();
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
        Json::Value ans;
        for (const auto& it : vec) {
          Json::Value thread;
          thread["title"] = clustering[it.second].GetTitle();
          Json::Value articles;
          for (const auto& d : clustering[it.second].GetDocs()) {
            articles.append(d.FileName);
          }
          thread["articles"] = std::move(articles);
          ans.append(thread);
        }
        Answers[durIdx][catIdx][langIdx] = ans;
      }
    }
  }
  /*for (const auto& cluster : clustering) {
    Answers[static_cast<size_t>(cluster.GetCategory())].push_back(cluster);
  }
  for (size_t idx = 0; idx < Answers.size(); ++idx) {
    std::sort(Answers[idx].begin(), Answers[idx].end(), [](const auto& l, const auto&r) {return l.GetTime() > r.GetTime();});
  }*/
}

Json::Value CalculatedResponses::GetAns(const std::string& lang, const std::string& category, const uint64_t period) {
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

CalculatedResponses ResponseBuilder::AddDocuments(const std::vector<DocumentConstPtr>& docs) {
  for (const auto& doc : docs) {
    Docs.emplace_back(*doc);
  }
  {
    for (auto& doc : Docs) {
      doc.ParseLang(Context.LangDetect.get());
      doc.Tokenize(Context);
      doc.DetectCategory(Context);
      doc.CalcWeight(Context);
    }
  }
  auto ruEmbedder = Embedder(Context.RuCatModel.get(), Context.RuMatrix, Context.RuBias);
  auto enEmbedder = Embedder(Context.EnCatModel.get(), Context.EnMatrix, Context.EnBias);
  {
    for (auto& doc : Docs) {
      if (doc.Lang && doc.Lang == "ru") {
        doc.Vector = ruEmbedder.GetEmbedding(doc);
      } else if (doc.Lang && doc.Lang == "en") {
        doc.Vector = enEmbedder.GetEmbedding(doc);
      }
    }
    std::vector<Cluster> clustering = RunClustering(Docs);
    return {Docs, clustering};
  }
}

}

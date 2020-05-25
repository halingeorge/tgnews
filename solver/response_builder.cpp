#include "response_builder.h"

#include <chrono>
#include <exception>
#include <string>
#include <vector>

#include "embedder.h"

#include <glog/logging.h>

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
#define ADD_TO_RESULT(s)   \
  {                        \
    nlohmann::json tmp;    \
    tmp["category"] = #s;  \
    tmp["articles"] = s;   \
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
  std::sort(weights.begin(), weights.end(),
            std::greater<std::pair<float, size_t>>());
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

}  // namespace

namespace tgnews {

CalculatedResponses::CalculatedResponses(const std::string& path) {
  std::ifstream ss(path);
  ss >> LangAns;
  ss >> NewsAns;
  ss >> CategoryAns;
  ss >> ThreadsAns;
  for (size_t durIdx = 0; durIdx < DiscretizationSize; ++durIdx) {
    for (size_t catIdx = 0; catIdx < NC_COUNT; ++catIdx) {
      for (size_t langIdx = 0; langIdx < LangCount; ++langIdx) {
        ss >> Answers[durIdx][catIdx][langIdx];
      }
    }
  }
}

void CalculatedResponses::dump(const std::string& path) {
  std::ofstream ss(path);
  ss << LangAns;
  ss << NewsAns;
  ss << CategoryAns;
  ss << ThreadsAns;
  for (size_t durIdx = 0; durIdx < DiscretizationSize; ++durIdx) {
    for (size_t catIdx = 0; catIdx < NC_COUNT; ++catIdx) {
      for (size_t langIdx = 0; langIdx < LangCount; ++langIdx) {
        ss << Answers[durIdx][catIdx][langIdx];
      }
    }
  }
}

CalculatedResponses::CalculatedResponses(
    const std::vector<tgnews::ParsedDoc>& docs,
    const std::vector<Cluster>& clustering) {
  LangAns = CalcLangAns(docs);
  NewsAns = CalcNewsAns(docs);
  CategoryAns = CalcCategoryAns(docs);
  ThreadsAns = CalcThreadsAns(clustering);
  uint64_t now = 0;
  if (clustering.size()) {
    now = std::max_element(clustering.begin(), clustering.end(),
                           [](const auto& l, const auto& r) {
                             return l.GetTime() < r.GetTime();
                           })
              ->GetTime();
  size_t durIdx = 0;
  for (const auto& duration : Discretization) {
    using WeightWithIdx = std::pair<float, size_t>;
    std::array<std::array<std::vector<WeightWithIdx>, ELang::LangCount>,
               ENewsCategory::NC_COUNT>
        weights;
    for (size_t clusterIdx = 0; clusterIdx < clustering.size(); ++clusterIdx) {
      const auto& c = clustering[clusterIdx];
      if (c.GetTime() + 2 * duration < now) {
        continue;  // cause clusters sorted (by freshness)
      }
      size_t catIdx = static_cast<size_t>(c.GetCategory());
      size_t langIdx = static_cast<size_t>(c.GetEnumLang());
      float clusterWeight = c.Weight();
      if (c.GetTime() + duration >= now) {
        clusterWeight *= 1.5;
      }
      weights[catIdx][langIdx].push_back({clusterWeight, clusterIdx});
      weights[static_cast<size_t>(NC_ANY)][langIdx].push_back(
          {clusterWeight, clusterIdx});
    }
    for (size_t catIdx = 0; catIdx < NC_COUNT; catIdx++) {
      for (size_t langIdx = 0; langIdx < LangCount; ++langIdx) {
        auto& vec = weights[catIdx][langIdx];
        std::sort(vec.begin(), vec.end(), std::greater<WeightWithIdx>());
        nlohmann::json threads = nlohmann::json::array();
        for (const auto& it : vec) {
          nlohmann::json thread;
          thread["title"] = clustering[it.second].GetTitle();
          thread["category"] = CategoryNames.at(static_cast<size_t>(clustering[it.second].GetCategory()));
          nlohmann::json articles = nlohmann::json::array();
          for (const auto& d : clustering[it.second].GetDocs()) {
            articles.push_back(d.FileName);
          }
          thread["articles"] = std::move(articles);
          threads.push_back(thread);
        }
        nlohmann::json ans = { {"threads",  threads} };
        Answers[durIdx][catIdx][langIdx] = std::move(ans);
      }
    }
    ++durIdx;
  }
  } else {
    std::cerr << "empty clustering somehow"; 
  }
}

nlohmann::json CalculatedResponses::GetAns(const std::string& lang,
                                           const std::string& category,
                                           const uint64_t period) {
  size_t idx = std::distance(
      Discretization.begin(),
      std::upper_bound(Discretization.begin(), Discretization.end(), period));
  if (idx == Discretization.size()) {
    --idx;
  }
  LOG(INFO) << idx << " - period idx";
  size_t catIdx = NC_COUNT;
  if (category == "any") catIdx = NC_ANY;
  if (category == "society") catIdx = NC_SOCIETY;
  if (category == "economy") catIdx = NC_ECONOMY;
  if (category == "technology") catIdx = NC_TECHNOLOGY;
  if (category == "sports") catIdx = NC_SPORTS;
  if (category == "entertainment") catIdx = NC_ENTERTAINMENT;
  if (category == "science") catIdx = NC_SCIENCE;
  if (category == "other") catIdx = NC_OTHER;
  LOG(INFO) << catIdx << " - cat idx";
  if (catIdx == NC_COUNT) {
    throw std::runtime_error("unknown cat");
  }
  size_t langIdx = LangCount;
  if (lang == "ru") langIdx = LangRu;
  if (lang == "en") langIdx = LangEn;
  LOG(INFO) << langIdx << " - lang idx";
  if (langIdx == LangCount) {
    throw std::runtime_error("unknown lang");
  }
  return Answers[idx][catIdx][langIdx];
}


ResponseBuilder::ResponseBuilder(tgnews::Context* context)
    : Context(context) {}

CalculatedResponses ResponseBuilder::AddDocuments(std::vector<ParsedDoc> docs) {
  LOG(INFO) << docs.size() << " - docs size";
  for (auto&& doc : docs) {
    if (doc.State == ParsedDoc::EState::Added) {
      Docs.push_back(std::move(doc));
    } else if (doc.State == ParsedDoc::EState::Removed) {
      size_t idx = std::distance(Docs.begin(), std::find_if(Docs.begin(), Docs.end(), [doc](const auto& d) { return d.FileName == doc.FileName;}));
      if (idx != Docs.size()) {
        std::swap(Docs[idx], Docs[Docs.size() - 1]);
        Docs.pop_back();
      }
    } else if (doc.State == ParsedDoc::EState::Changed) {
      size_t idx = std::distance(Docs.begin(), std::find_if(Docs.begin(), Docs.end(), [doc](const auto& d) { return d.FileName == doc.FileName;}));
      Docs[idx] = doc;
    }
  }
  {
    std::chrono::steady_clock::time_point begin =
        std::chrono::steady_clock::now();
    for (auto& doc : Docs) {
      doc.ParseLang(Context->LangDetect.get());
      doc.Tokenize(*Context);
      doc.DetectCategory(*Context);
      doc.CalcWeight(*Context);
    }
    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    LOG(INFO) << "Time difference categorization = "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       begin)
                     .count()
              << "[milli]";
  }
  auto ruEmbedder =
      Embedder(Context->RuCatModel.get(), Context->RuMatrix, Context->RuBias);
  auto enEmbedder =
      Embedder(Context->EnCatModel.get(), Context->EnMatrix, Context->EnBias);
  {
    std::chrono::steady_clock::time_point begin =
        std::chrono::steady_clock::now();
    for (auto& doc : Docs) {
      if (doc.Lang.size() && doc.Lang == "ru") {
        doc.Vector = ruEmbedder.GetEmbedding(doc);
      } else if (doc.Lang.size() && doc.Lang == "en") {
        doc.Vector = enEmbedder.GetEmbedding(doc);
      }
    }
    std::vector<Cluster> clustering = RunClustering(Docs);
    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    LOG(INFO) << "Time difference clustering = "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       begin)
                     .count()
              << "[milli]";
    return {Docs, clustering};
  }
}

}  // namespace tgnews

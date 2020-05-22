#include "response_builder.h"
#include "embedder.h"

#include <vector>
#include <string>
#include <chrono>
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
    if (doc.Category != ENewsCategory::NC_NOT_NEWS && doc.Category != ENewsCategory::NC_UNDEFINED && doc.Category != ENewsCategory::NC_OTHER) {
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
      articles.append(d.Title);
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
}

Json::Value CalculatedResponses::GetAns(const std::string& lang, const std::string& category, const uint64_t period) {
  return Json::Value{};
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
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    return {Docs, clustering};
  }
}

}

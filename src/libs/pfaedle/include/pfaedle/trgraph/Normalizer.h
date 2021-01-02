// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_NORMALIZER_H_
#define PFAEDLE_TRGRAPH_NORMALIZER_H_

#include <regex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <mutex>

namespace pfaedle {
namespace trgraph {

typedef std::pair<std::string, std::string> ReplRule;
using ReplRules = std::vector<ReplRule>;

typedef std::pair<std::regex, std::string> ReplRuleComp;
using ReplRulesComp = std::vector<ReplRuleComp>;

/*
 * A class for normalizing station names
 */
class Normalizer {
 public:
  Normalizer() {}
  explicit Normalizer(const ReplRules& rules);

  // copy constructor
  Normalizer(const Normalizer& other);

  // assignment op
  Normalizer& operator=(Normalizer other);

  // Normalize sn, not thread safe
  std::string norm(const std::string& sn) const;
  // Normalize sn, thread safe
  std::string normTS(const std::string& sn) const;

  // Normalize sn based on the rules of this normalizer, uses the thread safe
  // version of norm() internally
  std::string operator()(std::string sn) const;
  bool operator==(const Normalizer& b) const;

 private:
  ReplRulesComp _rules;
  ReplRules _rulesOrig;
  mutable std::unordered_map<std::string, std::string> _cache;
  mutable std::mutex _mutex;

  void buildRules(const ReplRules& rules);
};
}  // namespace trgraph
}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_NORMALIZER_H_

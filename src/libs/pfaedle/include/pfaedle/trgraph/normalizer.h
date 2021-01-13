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

namespace pfaedle::trgraph
{

using ReplRule = std::pair<std::string, std::string>;
using ReplRules = std::vector<ReplRule>;

using ReplRuleComp = std::pair<std::regex, std::string>;
using ReplRulesComp = std::vector<ReplRuleComp>;

/*
 * A class for normalizing station names
 */
class normalizer
{
public:
    normalizer() = default;
    explicit normalizer(const ReplRules& rules);

    // copy constructor
    normalizer(const normalizer& other);

    // assignment op
    normalizer& operator=(normalizer other);

    // Normalize sn, not thread safe
    std::string norm(const std::string& sn) const;
    // Normalize sn, thread safe
    std::string normTS(const std::string& sn) const;

    // Normalize sn based on the rules of this normalizer, uses the thread safe
    // version of norm() internally
    std::string operator()(std::string sn) const;
    bool operator==(const normalizer& b) const;

private:
    ReplRulesComp _rules;
    ReplRules _rulesOrig;
    mutable std::unordered_map<std::string, std::string> _cache;
    mutable std::mutex _mutex;

    void buildRules(const ReplRules& rules);
};
}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_NORMALIZER_H_

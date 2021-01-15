// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/normalizer.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using pfaedle::trgraph::normalizer;

normalizer::normalizer(const ReplRules& rules) :
    _rulesOrig(rules)
{
    build_rules(rules);
}

normalizer::normalizer(const normalizer& other) :
    _rules(other._rules),
    _rulesOrig(other._rulesOrig),
    _cache(other._cache) {}

normalizer& normalizer::operator=(normalizer other)
{
    std::swap(this->_rules, other._rules);
    std::swap(this->_rulesOrig, other._rulesOrig);
    std::swap(this->_cache, other._cache);

    return *this;
}

std::string normalizer::operator()(std::string sn) const
{
    return normTS(sn);
}

std::string normalizer::normTS(const std::string& sn) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return norm(sn);
}

std::string normalizer::norm(const std::string& sn) const
{
    auto i = _cache.find(sn);
    if (i != _cache.end()) return i->second;

    std::string ret = sn;
    for (const auto& rule : _rules)
    {
        std::string tmp;
        std::regex_replace(std::back_inserter(tmp), ret.begin(), ret.end(),
                           rule.first, rule.second,
                           std::regex_constants::format_sed);
        std::swap(ret, tmp);
    }

    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);

    _cache[sn] = ret;

    return ret;
}

bool normalizer::operator==(const normalizer& b) const
{
    return _rulesOrig == b._rulesOrig;
}

void normalizer::build_rules(const ReplRules& rules)
{
    for (const auto& rule : rules)
    {
        try
        {
            _rules.push_back(ReplRuleComp(
                    std::regex(rule.first, std::regex::ECMAScript | std::regex::icase |
                                                   std::regex::optimize),
                    rule.second));
        }
        catch (const std::regex_error& e)
        {
            std::stringstream ss;
            ss << "'" << rule.first << "'"
               << ": " << e.what();
            throw std::runtime_error(ss.str());
        }
    }
}

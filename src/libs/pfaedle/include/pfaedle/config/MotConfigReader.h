// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_CONFIG_MOTCONFIGREADER_H_
#define PFAEDLE_CONFIG_MOTCONFIGREADER_H_

#include "configparser/config_file_parser.h"
#include "cppgtfs/gtfs/Route.h"
#include "pfaedle/config/MotConfig.h"
#include "pfaedle/osm/OsmBuilder.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace pfaedle::config
{

using ad::cppgtfs::gtfs::Route;

class MotConfigReader
{
public:
    MotConfigReader();
    explicit MotConfigReader(const std::vector<std::string>&& paths);
    void parse(const std::vector<std::string>& paths);

    const std::vector<MotConfig>& getConfigs() const;

private:
    std::vector<MotConfig> _cfgs;

    osm::KeyVal getKv(const std::string& kv) const;
    osm::FilterRule getFRule(const std::string& kv) const;

    trgraph::ReplRules getNormRules(const std::vector<std::string>& arr) const;
    osm::DeepAttrRule getDeepAttrRule(const std::string& rule) const;
    uint64_t getFlags(const std::set<string>& flags) const;
};
}// namespace pfaedle

#endif// PFAEDLE_CONFIG_MOTCONFIGREADER_H_

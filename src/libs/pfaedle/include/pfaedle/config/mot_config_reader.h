// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_CONFIG_MOTCONFIGREADER_H_
#define PFAEDLE_CONFIG_MOTCONFIGREADER_H_

#include "configparser/config_file_parser.h"
#include "pfaedle/config/mot_config.h"
#include "pfaedle/osm/osm_builder.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace pfaedle::config
{

class mot_config_reader
{
public:
    mot_config_reader();
    explicit mot_config_reader(const std::vector<std::string>&& paths);
    void parse(const std::vector<std::string>& paths);

    const std::vector<mot_config>& get_configs() const;

private:
    osm::key_val_pair getKv(const std::string& kv) const;
    osm::filter_rule getFRule(const std::string& kv) const;

    trgraph::ReplRules getNormRules(const std::vector<std::string>& arr) const;
    osm::deep_attribute_rule getDeepAttrRule(const std::string& rule) const;
    uint64_t getFlags(const std::set<std::string>& flags) const;

    std::vector<mot_config> configs_;
};
}// namespace pfaedle

#endif// PFAEDLE_CONFIG_MOTCONFIGREADER_H_

// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_CONFIG_MOTCONFIG_H_
#define PFAEDLE_CONFIG_MOTCONFIG_H_

#include "pfaedle/osm/OsmBuilder.h"
#include "pfaedle/router/Router.h"
#include <map>
#include <string>

namespace pfaedle::config
{

struct MotConfig
{
    router::MOTs mots;
    osm::OsmReadOpts osmBuildOpts;
    router::RoutingOptions routingOpts;
    std::map<std::string, std::string> unproced;
};

inline bool operator==(const MotConfig& a, const MotConfig& b)
{
    bool unproced_eq = a.unproced.size() == b.unproced.size();

    if(!unproced_eq) return unproced_eq;

    for (const auto& kv : a.unproced)
    {
        if (!b.unproced.count(kv.first) || b.unproced.find(kv.first)->second != kv.second)
        {
            unproced_eq = false;
            break;
        }
    }
    return a.osmBuildOpts == b.osmBuildOpts && a.routingOpts == b.routingOpts && unproced_eq;
}

}  // namespace pfaedle::config

#endif// PFAEDLE_CONFIG_MOTCONFIG_H_

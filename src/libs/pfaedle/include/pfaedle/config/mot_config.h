// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_CONFIG_MOTCONFIG_H_
#define PFAEDLE_CONFIG_MOTCONFIG_H_

#include "pfaedle/osm/osm_builder.h"
#include "pfaedle/router/router.h"
#include <map>
#include <string>

namespace pfaedle::config
{

struct mot_config
{
    router::route_type_set route_types;
    osm::osm_read_options osmBuildOpts;
    router::routing_options routingOpts;
    std::map<std::string, std::string> unproced;
};

inline bool operator==(const mot_config& a, const mot_config& b)
{
    bool unproced_eq = a.unproced.size() == b.unproced.size();

    if(!unproced_eq)
        return unproced_eq;

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

// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_OSMREADOPTS_H_
#define PFAEDLE_OSM_OSMREADOPTS_H_

#include <pfaedle/osm/osm.h>
#include <pfaedle/trgraph/graph.h>
#include <pfaedle/trgraph/normalizer.h>

#include <queue>
#include <unordered_set>
#include <string>
#include <unordered_map>
#include <map>
#include <utility>
#include <vector>
#include <set>

namespace pfaedle::osm
{

using attribute_key_set = std::unordered_set<std::string>;
using node_id_map = std::unordered_map<osmid, trgraph::node*>;
using node_id_multimap = std::unordered_map<osmid, std::set<trgraph::node*>>;
using edge_candidate = std::pair<double, trgraph::edge*>;
using edge_candidate_priority_queue = std::priority_queue<edge_candidate>;
using attribute_list = std::vector<std::string>;

using attribute_flag_pair = std::pair<std::string, uint64_t>;
using multi_attribute_map = std::unordered_map<std::string, std::map<std::string, uint64_t>>;


using edge_tracks = std::unordered_map<const trgraph::edge*, std::string>;

struct relation_list
{
    std::vector<attribute_map> rels;
    flat_relations flat;
};

enum filter_flags : uint64_t
{
    USE = 1,// dummy value
    REL_NO_DOWN = 2,
    NO_RELATIONS = 4,
    NO_WAYS = 8,
    NO_NODES = 16,
    MULT_VAL_MATCH = 32
};

class filter_rule
{
public:
    filter_rule() :
        kv("", "")
    {}
    std::pair<std::string, std::string> kv;
    std::set<std::string> flags;
};

inline bool operator==(const filter_rule& a, const filter_rule& b)
{
    return a.kv == b.kv && a.flags == b.flags;
}

class deep_attribute_rule
{
public:
    std::string attr;
    filter_rule relRule;
};

inline bool operator==(const deep_attribute_rule& a, const deep_attribute_rule& b)
{
    return a.attr == b.attr && a.relRule == b.relRule;
}

using deep_attribute_list = std::vector<deep_attribute_rule>;

class relation_line_rules
{
public:
    attribute_list sNameRule;
    attribute_list fromNameRule;
    attribute_list toNameRule;
};

inline bool operator==(const relation_line_rules& a, const relation_line_rules& b)
{
    return a.sNameRule == b.sNameRule &&
           a.fromNameRule == b.fromNameRule &&
           a.toNameRule == b.toNameRule;
}

class station_attribute_rules
{
public:
    deep_attribute_list nameRule;
    deep_attribute_list platformRule;
    deep_attribute_list idRule;
};

inline bool operator==(const station_attribute_rules& a, const station_attribute_rules& b)
{
    return a.nameRule == b.nameRule && a.platformRule == b.platformRule;
}

struct station_group_node_attribute_rule
{
    deep_attribute_rule attr;
    double maxDist;
};

inline bool operator==(const station_group_node_attribute_rule& a,
                       const station_group_node_attribute_rule& b)
{
    return a.attr == b.attr && a.maxDist == b.maxDist;
}

using station_attribute_groups = std::unordered_map<std::string, std::unordered_map<std::string, std::vector<trgraph::station_group*>>>;

class osm_read_options
{
public:
    osm_read_options() = default;

    multi_attribute_map noHupFilter;
    multi_attribute_map keepFilter;
    multi_attribute_map levelFilters[8];
    multi_attribute_map dropFilter;
    multi_attribute_map oneWayFilter;
    multi_attribute_map oneWayFilterRev;
    multi_attribute_map twoWayFilter;
    multi_attribute_map stationFilter;
    multi_attribute_map stationBlockerFilter;
    std::vector<station_group_node_attribute_rule> statGroupNAttrRules;

    trgraph::normalizer statNormzer;
    trgraph::normalizer lineNormzer;
    trgraph::normalizer trackNormzer;
    trgraph::normalizer idNormzer;

    relation_line_rules relLinerules;
    station_attribute_rules statAttrRules;

    deep_attribute_list edgePlatformRules;

    uint8_t maxSnapLevel;

    double maxAngleSnapReach;
    std::vector<double> maxSnapDistances;
    double maxSnapFallbackHeurDistance;
    double maxBlockDistance;

    double maxOsmStationDistance;

    // TODO(patrick): this is not implemented yet
    double levelSnapPunishFac[7] = {0, 0, 0, 0, 0, 0, 0};

    double fullTurnAngle;

    // restriction system
    multi_attribute_map restrPosRestr;
    multi_attribute_map restrNegRestr;
    multi_attribute_map noRestrFilter;


    std::vector<attribute_key_set> get_kept_attribute_keys() const
    {
        std::vector<attribute_key_set> sets(3);
        for (const auto& i : statGroupNAttrRules)
        {
            if (i.attr.relRule.kv.first.empty())
            {
                sets[0].insert(i.attr.attr);
            }
            else
            {
                sets[2].insert(i.attr.relRule.kv.first);
                sets[2].insert(i.attr.attr);
            }
        }

        for (const auto& i : keepFilter)
        {
            for (auto & set : sets)
                set.insert(i.first);
        }

        for (const auto& i : dropFilter)
        {
            for (auto & set : sets) set.insert(i.first);
        }

        for (const auto& i : noHupFilter)
        {
            sets[0].insert(i.first);
        }

        for (const auto& i : oneWayFilter)
        {
            sets[1].insert(i.first);
        }

        for (const auto& i : oneWayFilterRev)
        {
            sets[1].insert(i.first);
        }

        for (const auto& i : twoWayFilter)
        {
            sets[1].insert(i.first);
        }

        for (const auto& i : stationFilter)
        {
            sets[0].insert(i.first);
            sets[2].insert(i.first);
        }

        for (const auto& i : stationBlockerFilter)
        {
            sets[0].insert(i.first);
        }

        for (uint8_t j = 0; j < 7; j++)
        {
            for (const auto& kv : *(levelFilters + j))
            {
                sets[1].insert(kv.first);
            }
        }

        // restriction system
        for (const auto& i : restrPosRestr)
        {
            sets[2].insert(i.first);
        }
        for (const auto& i : restrNegRestr)
        {
            sets[2].insert(i.first);
        }
        for (const auto& i : noRestrFilter)
        {
            sets[2].insert(i.first);
        }

        sets[1].insert("maxspeed");
        sets[2].insert("from");
        sets[2].insert("via");
        sets[2].insert("to");

        sets[2].insert(relLinerules.toNameRule.begin(), relLinerules.toNameRule.end());
        sets[2].insert(relLinerules.fromNameRule.begin(), relLinerules.fromNameRule.end());
        sets[2].insert(relLinerules.sNameRule.begin(), relLinerules.sNameRule.end());

        for (const auto& i : statAttrRules.nameRule)
        {
            if (i.relRule.kv.first.empty())
            {
                sets[0].insert(i.attr);
            }
            else
            {
                sets[2].insert(i.relRule.kv.first);
                sets[2].insert(i.attr);
            }
        }

        for (const auto& i : edgePlatformRules)
        {
            if (i.relRule.kv.first.empty())
            {
                sets[1].insert(i.attr);
            }
            else
            {
                sets[2].insert(i.relRule.kv.first);
                sets[2].insert(i.attr);
            }
        }

        for (const auto& i : statAttrRules.platformRule)
        {
            if (i.relRule.kv.first.empty())
            {
                sets[0].insert(i.attr);
            }
            else
            {
                sets[2].insert(i.relRule.kv.first);
                sets[2].insert(i.attr);
            }
        }

        for (const auto& i : statAttrRules.idRule)
        {
            if (i.relRule.kv.first.empty())
            {
                sets[0].insert(i.attr);
            }
            else
            {
                sets[2].insert(i.relRule.kv.first);
                sets[2].insert(i.attr);
            }
        }

        return sets;
    }


};

inline bool operator==(const osm_read_options& a, const osm_read_options& b)
{
    if (a.maxSnapDistances.size() != b.maxSnapDistances.size()) return false;
    for (size_t i = 0; i < a.maxSnapDistances.size(); i++)
    {
        if (fabs(a.maxSnapDistances[i] - b.maxSnapDistances[i]) >= 0.1)
            return false;
    }

    return a.noHupFilter == b.noHupFilter && a.keepFilter == b.keepFilter &&
           a.levelFilters[0] == b.levelFilters[0] &&
           a.levelFilters[1] == b.levelFilters[1] &&
           a.levelFilters[2] == b.levelFilters[2] &&
           a.levelFilters[3] == b.levelFilters[3] &&
           a.levelFilters[4] == b.levelFilters[4] &&
           a.levelFilters[5] == b.levelFilters[5] &&
           a.levelFilters[6] == b.levelFilters[6] &&
           a.dropFilter == b.dropFilter && a.oneWayFilter == b.oneWayFilter &&
           a.oneWayFilterRev == b.oneWayFilterRev &&
           a.twoWayFilter == b.twoWayFilter &&
           a.stationFilter == b.stationFilter &&
           a.stationBlockerFilter == b.stationBlockerFilter &&
           a.statGroupNAttrRules == b.statGroupNAttrRules &&
           a.statNormzer == b.statNormzer && a.lineNormzer == b.lineNormzer &&
           a.trackNormzer == b.trackNormzer && a.relLinerules == b.relLinerules &&
           a.statAttrRules == b.statAttrRules &&
           a.maxSnapLevel == b.maxSnapLevel &&
           fabs(a.maxAngleSnapReach - b.maxAngleSnapReach) < 0.1 &&
           fabs(a.maxOsmStationDistance - b.maxOsmStationDistance) < 0.1 &&
           fabs(a.maxSnapFallbackHeurDistance - b.maxSnapFallbackHeurDistance) <
                   0.1 &&
           fabs(a.maxBlockDistance - b.maxBlockDistance) < 0.1 &&
           fabs(a.levelSnapPunishFac[0] - b.levelSnapPunishFac[0]) < 0.1 &&
           fabs(a.levelSnapPunishFac[1] - b.levelSnapPunishFac[1]) < 0.1 &&
           fabs(a.levelSnapPunishFac[2] - b.levelSnapPunishFac[2]) < 0.1 &&
           fabs(a.levelSnapPunishFac[3] - b.levelSnapPunishFac[3]) < 0.1 &&
           fabs(a.levelSnapPunishFac[4] - b.levelSnapPunishFac[4]) < 0.1 &&
           fabs(a.levelSnapPunishFac[5] - b.levelSnapPunishFac[5]) < 0.1 &&
           fabs(a.levelSnapPunishFac[6] - b.levelSnapPunishFac[6]) < 0.1 &&
           fabs(a.fullTurnAngle - b.fullTurnAngle) < 0.1 &&
           a.restrPosRestr == b.restrPosRestr &&
           a.restrNegRestr == b.restrNegRestr &&
           a.noRestrFilter == b.noRestrFilter;
}
}  // namespace pfaedle::osm
#endif  // PFAEDLE_OSM_OSMREADOPTS_H_

// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_OSMFILTER_H_
#define PFAEDLE_OSM_OSMFILTER_H_

#include "pfaedle/osm/osm.h"
#include "pfaedle/osm/osm_read_options.h"
#include <string>
#include <vector>

namespace pfaedle::osm
{

class osm_filter
{
public:
    enum Type : uint64_t
    {
        NODE = 16,
        WAY = 8,
        REL = 4,
        ALL = 0
    };
    osm_filter() = default;
    osm_filter(const multi_attribute_map& keep, const multi_attribute_map& drop);
    explicit osm_filter(const osm_read_options& o);
    uint64_t keep(const attribute_map& attrs, Type t) const;
    uint64_t drop(const attribute_map& attrs, Type t) const;
    uint64_t nohup(const char* key, const char* val) const;
    uint8_t level(const attribute_map& attrs) const;
    uint64_t oneway(const attribute_map& attrs) const;
    uint64_t onewayrev(const attribute_map& attrs) const;
    uint64_t station(const attribute_map& attrs) const;
    uint64_t blocker(const attribute_map& attrs) const;
    uint64_t negRestr(const attribute_map& attrs) const;
    uint64_t posRestr(const attribute_map& attrs) const;
    std::vector<std::string> getAttrKeys() const;

    osm_filter merge(const osm_filter& other) const;

    const multi_attribute_map& getKeepRules() const;
    const multi_attribute_map& getDropRules() const;

    std::string toString() const;

    static bool valMatches(const std::string& a, const std::string& b, bool m);
    static bool valMatches(const std::string& a, const std::string& b);
    static uint64_t contained(const attribute_map& attrs, const multi_attribute_map& map,
                              Type t);
    static uint64_t contained(const attribute_map& attrs, const attribute& map);

private:
    multi_attribute_map _keep;
    multi_attribute_map _drop;
    multi_attribute_map _nohup;
    multi_attribute_map _oneway;
    multi_attribute_map _onewayrev;
    multi_attribute_map _twoway;
    multi_attribute_map _station;
    multi_attribute_map _blocker;
    multi_attribute_map _posRestr;
    multi_attribute_map _negRestr;
    multi_attribute_map _noRestr;
    const multi_attribute_map* _levels;
};
}  // namespace pfaedle
#endif  // PFAEDLE_OSM_OSMFILTER_H_

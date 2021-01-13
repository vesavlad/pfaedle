// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_OSM_OSM_H_
#define PFAEDLE_OSM_OSM_H_

#include <set>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

namespace pfaedle::osm
{

using osmid = uint64_t;

using attribute_map = std::unordered_map<std::string, std::string>;
using attribute = std::pair<std::string, std::string>;
using osmid_list = std::vector<osmid>;

struct osm_element
{
    osmid id;
    attribute_map attrs;
};

struct osm_relation : public osm_element
{
    std::vector<osmid> nodes;
    std::vector<osmid> ways;

    std::vector<std::string> nodeRoles;
    std::vector<std::string> wayRoles;

    uint64_t keepFlags;
    uint64_t dropFlags;
};

struct osm_way : public osm_element
{
    std::vector<osmid> nodes;

    uint64_t keepFlags;
    uint64_t dropFlags;
};

struct osm_node : public osm_element
{
    double lat;
    double lng;

    uint64_t keepFlags;
    uint64_t dropFlags;
};

struct restriction
{
    restriction(osmid from, osmid to):
        eFrom{from},
        eTo{to}
    {}
    osmid eFrom;
    osmid eTo;
};

using restriction_map = std::unordered_map<osmid, std::vector<restriction>>;

struct restrictions
{
    restriction_map pos;
    restriction_map neg;
};
}  // namespace pfaedle::osm
#endif  // PFAEDLE_OSM_OSM_H_

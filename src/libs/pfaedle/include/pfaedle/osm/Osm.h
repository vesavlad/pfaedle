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

using AttrMap = std::unordered_map<std::string, std::string>;
using Attr = std::pair<std::string, std::string>;
using OsmIdList = std::vector<osmid>;

struct OsmElement
{
    osmid id;
    AttrMap attrs;
};

struct OsmRel: public OsmElement
{
    std::vector<osmid> nodes;
    std::vector<osmid> ways;

    std::vector<std::string> nodeRoles;
    std::vector<std::string> wayRoles;

    uint64_t keepFlags;
    uint64_t dropFlags;
};

struct OsmWay: public OsmElement
{
    std::vector<osmid> nodes;

    uint64_t keepFlags;
    uint64_t dropFlags;
};

struct OsmNode: public OsmElement
{
    double lat;
    double lng;

    uint64_t keepFlags;
    uint64_t dropFlags;
};

struct Restriction
{
    Restriction(osmid from, osmid to):
        eFrom{from},
        eTo{to}
    {}
    osmid eFrom;
    osmid eTo;
};

using RestrMap = std::unordered_map<osmid, std::vector<Restriction>>;

struct Restrictions
{
    RestrMap pos;
    RestrMap neg;
};
}  // namespace pfaedle::osm
#endif  // PFAEDLE_OSM_OSM_H_

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

namespace pfaedle {
namespace osm {

using osmid = uint64_t;

typedef std::unordered_map<std::string, std::string> AttrMap;
typedef std::pair<std::string, std::string> Attr;
using OsmIdList = std::vector<osmid>;

struct OsmRel {
  OsmRel() : id(0) {}
  osmid id;
  AttrMap attrs;
  std::vector<osmid> nodes;
  std::vector<osmid> ways;

  std::vector<std::string> nodeRoles;
  std::vector<std::string> wayRoles;

  uint64_t keepFlags;
  uint64_t dropFlags;
};

struct OsmWay {
  OsmWay() : id(0) {}
  osmid id;
  AttrMap attrs;
  std::vector<osmid> nodes;

  uint64_t keepFlags;
  uint64_t dropFlags;
};

struct OsmNode {
  OsmNode() : id(0) {}
  osmid id;
  double lat;
  double lng;
  AttrMap attrs;

  uint64_t keepFlags;
  uint64_t dropFlags;
};

struct Restriction {
  osmid eFrom, eTo;
};

typedef std::unordered_map<osmid, std::vector<Restriction>> RestrMap;

struct Restrictions {
  RestrMap pos;
  RestrMap neg;
};
}  // namespace osm
}  // namespace pfaedle
#endif  // PFAEDLE_OSM_OSM_H_

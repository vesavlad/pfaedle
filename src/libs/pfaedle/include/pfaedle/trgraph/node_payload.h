// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_NODEPL_H_
#define PFAEDLE_TRGRAPH_NODEPL_H_

#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/definitions.h"
#include "pfaedle/trgraph/station_info.h"
#include "util/geo/Geo.h"
#include "util/geo/GeoGraph.h"
#include <map>
#include <string>
#include <unordered_map>

namespace pfaedle::trgraph
{

struct component
{
    uint8_t minEdgeLvl : 3;
};

/*
 * A node payload class for the transit graph.
 */
class node_payload
{
public:
    node_payload();
    node_payload(const node_payload& pl); // NOLINT
    node_payload(const POINT& geom);// NOLINT
    node_payload(const POINT& geom, const station_info& si);
    ~node_payload();

    // Return the geometry of this node.
    const POINT* get_geom() const;
    void set_geom(const POINT& geom);

    // Fill obj with k/v pairs describing the parameters of this payload.
    util::json::Dict get_attrs() const;

    // Set the station info for this node
    void set_si(const station_info& si);

    // Return the station info for this node
    const station_info* get_si() const;
    station_info* get_si();

    // Delete the station info for this node
    void set_no_statistics();

    // Get the component of this node
    const component* get_component() const;

    // Set the component of this node
    void set_component(const component* c);

    // Make this node a blocker
    void set_blocker();

    // Check if this node is a blocker
    bool is_blocker() const;

    // Mark this node as visited (usefull for counting search space in Dijkstra)
    // (only works for DEBUG build type)
    void set_visited() const;

private:
    POINT _geom;
    station_info* _si;
    const component* _component;

#ifdef PFAEDLE_DBG
    mutable bool _vis;
#endif

    static station_info _blockerSI;
    static std::unordered_map<const component*, size_t> _comps;
};
}  // namespace pfaedle::trgraph

#endif  // PFAEDLE_TRGRAPH_NODEPL_H_

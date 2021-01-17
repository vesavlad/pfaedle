// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_EDGEPL_H_
#define PFAEDLE_TRGRAPH_EDGEPL_H_

#include "pfaedle/definitions.h"
#include "pfaedle/router/comp.h"
#include "util/geo/Geo.h"
#include "util/geo/GeoGraph.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace pfaedle::trgraph
{

/*
 * A line occuring on an edge
 */
struct transit_edge_line
{
    std::string fromStr;
    std::string toStr;
    std::string shortName;
};

inline bool operator==(const transit_edge_line& a, const transit_edge_line& b)
{
    return a.fromStr == b.fromStr && a.toStr == b.toStr &&
           a.shortName == b.shortName;
}

inline bool operator<(const transit_edge_line& a, const transit_edge_line& b)
{
    return a.fromStr < b.fromStr ||
           (a.fromStr == b.fromStr && a.toStr < b.toStr) ||
           (a.fromStr == b.fromStr && a.toStr == b.toStr &&
            a.shortName < b.shortName);
}

/*
 * An edge payload class for the transit graph.
 */
class edge_payload
{
public:
    edge_payload();
    ~edge_payload();
    edge_payload(const edge_payload& pl);
    edge_payload(const edge_payload& pl, bool geoFlat);

    // Return the geometry of this edge.
    const LINE* get_geom() const;
    LINE* get_geom();

    // Extends this edge payload's geometry by Point p
    void add_point(const POINT& p);

    // Fill obj with k/v pairs describing the parameters of this payload.
    util::json::Dict get_attrs() const;

    // Return the length in meters stored for this edge payload
    double get_length() const;

    // Set the length in meters for this edge payload
    void set_length(double d);

    // Return the max speed in km/h stored for this edge payload
    double get_max_speed() const;

    // Set the length in km/h for this edge payload
    void set_max_speed(double speed);

    // Set this edge as a one way node, either in the default direction of
    // the edge (no arg), or the direction specified in dir
    void setOneWay();
    void setOneWay(uint8_t dir);

    // Mark this payload' edge as having some restrictions
    void set_restricted();

    // Mark this payload' edge as being secondary to an inversed partner
    void set_reversed();

    // True if this edge is secondary to an inversed partner
    bool is_reversed() const;

    // True if this edge is restricted
    bool is_restricted() const;

    // Set the level of this edge.
    void set_level(uint8_t lvl);

    // Return the level of this edge.
    uint8_t level() const;

    // Return the one-way code stored for this edge.
    uint8_t oneWay() const;

    // Add a TransitedgeLine to this payload's edge
    void add_line(const transit_edge_line* l);

    // Add multiple TransitedgeLine objects to this payload's edge
    void add_lines(const std::vector<transit_edge_line*>& l);

    // Return the TransitEdgeLines stored for this payload
    const std::vector<const transit_edge_line*>& get_lines() const;

    // Returns the last hop of the payload - this is the (n-2)th point in
    // the payload geometry of length n > 1
    const POINT& backHop() const;

    // Returns the first hop of the payload - this is the 2nd point in
    // the payload geometry of length n > 1
    const POINT& frontHop() const;

    // Obtain an exact copy of this edge, but in reverse.
    edge_payload revCopy() const;

private:
    float _length;
    double _max_speed;
    uint8_t _oneWay : 2;
    bool _hasRestr : 1;
    bool _rev : 1;
    uint8_t _lvl : 3;

    LINE* _l;

    std::vector<const transit_edge_line*> _lines;

    static void unRefTLine(const transit_edge_line* l);

    static std::map<LINE*, size_t> _flines;
    static std::map<const transit_edge_line*, size_t> _tlines;
};
}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_EDGEPL_H_

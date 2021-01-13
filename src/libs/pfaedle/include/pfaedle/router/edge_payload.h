// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_EDGEPL_H_
#define PFAEDLE_ROUTER_EDGEPL_H_

#include <map>
#include <pfaedle/definitions.h>
#include <pfaedle/router/misc.h>
#include <string>
#include <util/geo/Geo.h>
#include <util/geo/GeoGraph.h>

namespace pfaedle::router
{
class edge_payload
{
public:
    edge_payload() :
        _cost(),
        _start(nullptr),
        _end(nullptr),
        _startE(nullptr),
        _endE(nullptr)
    {}

    const LINE* get_geom() const;
    util::json::Dict get_attrs() const;
    router::edge_list* get_edges();
    const router::edge_list& get_edges() const;

    void set_start_node(const trgraph::node* s);
    void set_end_node(const trgraph::node* s);
    void set_start_edge(const trgraph::edge* s);
    void set_end_edge(const trgraph::edge* s);

    const router::edge_cost& get_cost() const;
    void set_cost(const router::edge_cost& c);

    const POINT& frontHop() const;
    const POINT& backHop() const;

    const trgraph::node* front_node() const;
    const trgraph::node* back_node() const;

private:
    router::edge_cost _cost;
    // the edges are in this field in REVERSED ORDER!
    router::edge_list _edges;
    const trgraph::node* _start;
    const trgraph::node* _end;
    const trgraph::edge* _startE;
    const trgraph::edge* _endE;
    mutable LINE _geom;
};
}  // namespace pfaedle

#endif  // PFAEDLE_ROUTER_EDGEPL_H_

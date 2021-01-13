// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_EDGEPL_H_
#define PFAEDLE_ROUTER_EDGEPL_H_

#include <map>
#include <pfaedle/Def.h>
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

    const LINE* getGeom() const;
    util::json::Dict getAttrs() const;
    router::edge_list* getEdges();
    const router::edge_list& getEdges() const;

    void setStartNode(const trgraph::Node* s);
    void setEndNode(const trgraph::Node* s);
    void setStartEdge(const trgraph::Edge* s);
    void setEndEdge(const trgraph::Edge* s);

    const router::edge_cost& getCost() const;
    void setCost(const router::edge_cost& c);

    const POINT& frontHop() const;
    const POINT& backHop() const;

    const trgraph::Node* frontNode() const;
    const trgraph::Node* backNode() const;

private:
    router::edge_cost _cost;
    // the edges are in this field in REVERSED ORDER!
    router::edge_list _edges;
    const trgraph::Node* _start;
    const trgraph::Node* _end;
    const trgraph::Edge* _startE;
    const trgraph::Edge* _endE;
    mutable LINE _geom;
};
}  // namespace pfaedle

#endif  // PFAEDLE_ROUTER_EDGEPL_H_

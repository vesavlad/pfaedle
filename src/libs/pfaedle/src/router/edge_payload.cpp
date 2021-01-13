// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/router/edge_payload.h"
#include "pfaedle/Def.h"
#include "util/String.h"

namespace pfaedle::router
{
edge_list* edge_payload::getEdges() { return &_edges; }

const edge_list& edge_payload::getEdges() const { return _edges; }

const POINT& edge_payload::frontHop() const
{
    if (_edges.empty()) return *_end->pl().getGeom();
    return _edges.back()->pl().frontHop();
}

const POINT& edge_payload::backHop() const
{
    if (_edges.empty()) return *_start->pl().getGeom();
    return _edges.front()->pl().backHop();
}

const trgraph::Node* edge_payload::backNode() const
{
    return _end;
}

const trgraph::Node* edge_payload::frontNode() const
{
    return _start;
}

const LINE* edge_payload::getGeom() const
{
    if (_edges.empty()) return nullptr;
    if (_geom.empty())
    {
        const trgraph::Node* l = _start;
        for (auto i = _edges.rbegin(); i != _edges.rend(); i++)
        {
            const auto e = *i;
            if ((e->getFrom() == l) ^ e->pl().isRev())
            {
                _geom.insert(_geom.end(), e->pl().getGeom()->begin(),
                             e->pl().getGeom()->end());
            }
            else
            {
                _geom.insert(_geom.end(), e->pl().getGeom()->rbegin(),
                             e->pl().getGeom()->rend());
            }
            l = e->getOtherNd(l);
        }
    }

    return &_geom;
}

void edge_payload::setStartNode(const trgraph::Node* s) { _start = s; }

void edge_payload::setEndNode(const trgraph::Node* e) { _end = e; }

void edge_payload::setStartEdge(const trgraph::Edge* s) { _startE = s; }

void edge_payload::setEndEdge(const trgraph::Edge* e) { _endE = e; }

const edge_cost& edge_payload::getCost() const { return _cost; }

void edge_payload::setCost(const router::edge_cost& c) { _cost = c; }

util::json::Dict edge_payload::getAttrs() const
{
    util::json::Dict obj;
    obj["cost"] = std::to_string(_cost.getValue());
    obj["from_edge"] = util::toString(_startE);
    obj["to_edge"] = util::toString(_endE);
    obj["dummy"] = !_edges.empty() ? "no" : "yes";

    return obj;
}

}

// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/router/EdgePL.h"
#include "pfaedle/Def.h"
#include "pfaedle/router/Router.h"
#include "util/String.h"
#include "util/geo/Geo.h"

using pfaedle::router::EdgeCost;
using pfaedle::router::EdgeList;
using pfaedle::router::EdgePL;
using pfaedle::trgraph::Node;

// _____________________________________________________________________________
EdgeList* EdgePL::getEdges() { return &_edges; }

// _____________________________________________________________________________
const EdgeList& EdgePL::getEdges() const { return _edges; }

// _____________________________________________________________________________
const POINT& EdgePL::frontHop() const
{
    if (!_edges.size()) return *_end->pl().getGeom();
    return _edges.back()->pl().frontHop();
}

// _____________________________________________________________________________
const POINT& EdgePL::backHop() const
{
    if (!_edges.size()) return *_start->pl().getGeom();
    return _edges.front()->pl().backHop();
}

// _____________________________________________________________________________
const Node* EdgePL::backNode() const { return _end; }

// _____________________________________________________________________________
const Node* EdgePL::frontNode() const { return _start; }

// _____________________________________________________________________________
const LINE* EdgePL::getGeom() const
{
    if (!_edges.size()) return nullptr;
    if (!_geom.size())
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

// _____________________________________________________________________________
void EdgePL::setStartNode(const trgraph::Node* s) { _start = s; }

// _____________________________________________________________________________
void EdgePL::setEndNode(const trgraph::Node* e) { _end = e; }

// _____________________________________________________________________________
void EdgePL::setStartEdge(const trgraph::Edge* s) { _startE = s; }

// _____________________________________________________________________________
void EdgePL::setEndEdge(const trgraph::Edge* e) { _endE = e; }

// _____________________________________________________________________________
const EdgeCost& EdgePL::getCost() const { return _cost; }

// _____________________________________________________________________________
void EdgePL::setCost(const router::EdgeCost& c) { _cost = c; }

// _____________________________________________________________________________
util::json::Dict EdgePL::getAttrs() const
{
    util::json::Dict obj;
    obj["cost"] = std::to_string(_cost.getValue());
    obj["from_edge"] = util::toString(_startE);
    obj["to_edge"] = util::toString(_endE);
    obj["dummy"] = _edges.size() ? "no" : "yes";

    return obj;
}

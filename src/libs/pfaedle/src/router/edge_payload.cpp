// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/router/edge_payload.h"
#include "pfaedle/definitions.h"
#include "util/String.h"

namespace pfaedle::router
{
edge_list* edge_payload::get_edges() { return &_edges; }

const edge_list& edge_payload::get_edges() const { return _edges; }

const POINT& edge_payload::frontHop() const
{
    if (_edges.empty())
        return *_end->pl().get_geom();
    return _edges.back()->pl().frontHop();
}

const POINT& edge_payload::backHop() const
{
    if (_edges.empty())
        return *_start->pl().get_geom();
    return _edges.front()->pl().backHop();
}

const trgraph::node* edge_payload::back_node() const
{
    return _end;
}

const trgraph::node* edge_payload::front_node() const
{
    return _start;
}

const LINE* edge_payload::get_geom() const
{
    if (_edges.empty()) return nullptr;
    if (_geom.empty())
    {
        const trgraph::node* l = _start;
        for (auto i = _edges.rbegin(); i != _edges.rend(); i++)
        {
            const auto e = *i;
            if ((e->getFrom() == l) ^ e->pl().is_reversed())
            {
                _geom.insert(_geom.end(), e->pl().get_geom()->begin(),
                             e->pl().get_geom()->end());
            }
            else
            {
                _geom.insert(_geom.end(), e->pl().get_geom()->rbegin(),
                             e->pl().get_geom()->rend());
            }
            l = e->getOtherNd(l);
        }
    }

    return &_geom;
}

void edge_payload::set_start_node(const trgraph::node* s) { _start = s; }

void edge_payload::set_end_node(const trgraph::node* s) { _end = s; }

void edge_payload::set_start_edge(const trgraph::edge* s) { _startE = s; }

void edge_payload::set_end_edge(const trgraph::edge* s) { _endE = s; }

const edge_cost& edge_payload::get_cost() const { return _cost; }

void edge_payload::set_cost(const router::edge_cost& c) { _cost = c; }

util::json::Dict edge_payload::get_attrs() const
{
    util::json::Dict obj;
    obj["cost"] = std::to_string(_cost.getValue());
    obj["from_edge"] = util::toString(_startE);
    obj["to_edge"] = util::toString(_endE);
    obj["dummy"] = !_edges.empty() ? "no" : "yes";

    return obj;
}

}

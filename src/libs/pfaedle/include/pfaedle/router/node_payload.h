// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_NODEPL_H_
#define PFAEDLE_ROUTER_NODEPL_H_

#include "pfaedle/definitions.h"
#include "pfaedle/trgraph/graph.h"
#include "util/geo/Geo.h"
#include "util/geo/GeoGraph.h"
#include <map>
#include <string>

namespace pfaedle::router
{

class node_payload
{
public:
    node_payload() :
        _n(nullptr)
    {}

    node_payload(const pfaedle::trgraph::node* n) :
        _n(n)
    {}

    const POINT* get_geom() const
    {
        return !_n ? nullptr : _n->pl().get_geom();
    }

    util::json::Dict get_attrs() const
    {
        if (_n)
        {
            return _n->pl().get_attrs();
        }
        return util::json::Dict();
    }

private:
    const pfaedle::trgraph::node* _n;
};
}  // namespace pfaedle

#endif  // PFAEDLE_ROUTER_NODEPL_H_

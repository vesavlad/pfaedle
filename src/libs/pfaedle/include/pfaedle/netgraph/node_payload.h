// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_NETGRAPH_NODEPL_H_
#define PFAEDLE_NETGRAPH_NODEPL_H_

#include <pfaedle/definitions.h>
#include <util/json/Writer.h>

namespace pfaedle::netgraph
{

/*
 * A payload class for edges on a network graph - that is a graph
 * that exactly represents a physical public transit network
 */
class node_payload
{
public:
    node_payload() = default;
    node_payload(const POINT& geom)
    {
        _geom = geom;
    }// NOLINT

    const POINT* getGeom() const
    {
        return &_geom;
    }
    util::json::Dict getAttrs() const
    {
        return util::json::Dict();
    }

private:
    POINT _geom;
};
}  // namespace pfaedle

#endif  // PFAEDLE_NETGRAPH_NODEPL_H_

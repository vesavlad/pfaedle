// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_GRAPH_H_
#define PFAEDLE_ROUTER_GRAPH_H_

#include <pfaedle/router/edge_payload.h>
#include <pfaedle/router/node_payload.h>
#include <pfaedle/trgraph/Graph.h>
#include <util/graph/DirGraph.h>


namespace pfaedle::router
{
using edge = util::graph::Edge<router::node_payload, router::edge_payload>;
using node = util::graph::Node<router::node_payload, router::edge_payload>;
using graph =  util::graph::DirGraph<router::node_payload, router::edge_payload>;
}  // namespace pfaedle

#endif  // PFAEDLE_ROUTER_GRAPH_H_

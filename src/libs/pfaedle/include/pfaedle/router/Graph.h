// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_GRAPH_H_
#define PFAEDLE_ROUTER_GRAPH_H_

#include <pfaedle/trgraph/Graph.h>
#include <pfaedle/router/EdgePL.h>
#include <pfaedle/router/NodePL.h>
#include <util/graph/DirGraph.h>


namespace pfaedle::router
{
using Edge = util::graph::Edge<router::NodePL, router::EdgePL>;
using Node = util::graph::Node<router::NodePL, router::EdgePL>;
using Graph =  util::graph::DirGraph<router::NodePL, router::EdgePL>;
}  // namespace pfaedle

#endif  // PFAEDLE_ROUTER_GRAPH_H_

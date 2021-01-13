// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_NETGRAPH_GRAPH_H_
#define PFAEDLE_NETGRAPH_GRAPH_H_

#include <pfaedle/netgraph/edge_payload.h>
#include <pfaedle/netgraph/node_payload.h>
#include <util/graph/UndirGraph.h>


namespace pfaedle::netgraph
{

/*
 * A payload class for edges on a network graph - that is a graph
 * that exactly represents a physical public transit network
 */
using edge = util::graph::Edge<node_payload, edge_payload>;
using node = util::graph::Node<node_payload, edge_payload>;
using graph = util::graph::UndirGraph<node_payload, edge_payload>;

}  // namespace pfaedle

#endif  // PFAEDLE_NETGRAPH_GRAPH_H_

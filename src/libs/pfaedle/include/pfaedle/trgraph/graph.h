// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_GRAPH_H_
#define PFAEDLE_TRGRAPH_GRAPH_H_

#include "pfaedle/trgraph/edge_payload.h"
#include "pfaedle/trgraph/node_payload.h"
#include "util/geo/Grid.h"
#include "util/graph/DirGraph.h"
#include "util/graph/UndirGraph.h"

namespace pfaedle::trgraph
{
/*
 * A graph for physical transit networks
 */
using edge = util::graph::Edge<node_payload, edge_payload>;
using node = util::graph::Node<node_payload, edge_payload>;
using graph = util::graph::DirGraph<node_payload, edge_payload>;
using node_grid = util::geo::Grid<node*, util::geo::Point, PFAEDLE_PRECISION>;
using edge_grid = util::geo::Grid<edge*, util::geo::Line, PFAEDLE_PRECISION>;

}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_GRAPH_H_

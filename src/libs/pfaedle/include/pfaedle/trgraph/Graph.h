// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_GRAPH_H_
#define PFAEDLE_TRGRAPH_GRAPH_H_

#include "pfaedle/trgraph/EdgePayload.h"
#include "pfaedle/trgraph/NodePayload.h"
#include "util/geo/Grid.h"
#include "util/graph/DirGraph.h"
#include "util/graph/UndirGraph.h"

namespace pfaedle::trgraph
{
/*
 * A graph for physical transit networks
 */
using Edge = util::graph::Edge<NodePayload, EdgePayload>;
using Node = util::graph::Node<NodePayload, EdgePayload>;
using Graph = util::graph::DirGraph<NodePayload, EdgePayload>;
using NodeGrid = util::geo::Grid<Node*, util::geo::Point, PFAEDLE_PRECISION>;
using EdgeGrid = util::geo::Grid<Edge*, util::geo::Line, PFAEDLE_PRECISION>;

}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_GRAPH_H_

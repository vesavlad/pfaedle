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

using util::geo::Grid;
using util::geo::Point;
using util::geo::Line;

namespace pfaedle {
namespace trgraph {

/*
 * A graph for physical transit networks
*/
typedef util::graph::Edge<NodePayload, EdgePayload> Edge;
typedef util::graph::Node<NodePayload, EdgePayload> Node;
typedef util::graph::DirGraph<NodePayload, EdgePayload> Graph;
typedef Grid<Node*, Point, PFAEDLE_PRECISION> NodeGrid;
typedef Grid<Edge*, Line, PFAEDLE_PRECISION> EdgeGrid;

}  // namespace trgraph
}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_GRAPH_H_

// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_TRGRAPH_GRAPH_H_
#define PFAEDLE_TRGRAPH_GRAPH_H_

#include "util/graph/DirGraph.h"
#include "util/graph/UndirGraph.h"

#include <pfaedle/trgraph/node_grid.h>
#include <pfaedle/trgraph/edge_grid.h>

namespace pfaedle::trgraph
{

class restrictor;
/*
 * A graph for physical transit networks
 */

class graph : public util::graph::DirGraph<node_payload, edge_payload>
{
public:
    void write_geometries();

    void delete_orphan_nodes();
    void delete_orphan_edges(double turn_angle);

    void collapse_edges();

    void simplify_geometries();

    uint32_t write_components();

    void writeSelfEdgs();

    void fix_gaps(trgraph::node_grid& ng);

    void writeODirEdgs(restrictor& restor);

private:
    static bool are_edges_similar(const edge& a, const edge& b);

    const edge_payload& merge_edge_payload(edge& a, edge& b);
};

}  // namespace pfaedle

#endif  // PFAEDLE_TRGRAPH_GRAPH_H_

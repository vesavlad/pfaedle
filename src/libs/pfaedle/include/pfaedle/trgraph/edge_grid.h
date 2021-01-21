#ifndef CONFIG_FILE_PARSER_H_EDGE_GRID_H
#define CONFIG_FILE_PARSER_H_EDGE_GRID_H

#include "pfaedle/trgraph/edge_payload.h"
#include "pfaedle/trgraph/node_payload.h"
#include <util/graph/Edge.h>
#include <util/geo/Grid.h>

#include <queue>
namespace pfaedle::trgraph
{
class graph;

using edge = util::graph::Edge<node_payload, edge_payload>;

class edge_grid : public util::geo::Grid<edge*, util::geo::Line, PFAEDLE_PRECISION>
{
public:
    using edge_candidate = std::pair<double, edge*>;
    using edge_candidate_priority_queue = std::priority_queue<edge_candidate>;

    static trgraph::edge_grid build_edge_grid(graph& g, size_t size, const BOX& webMercBox);
    // initialization of a point grid with cell width w and cell height h
    // that covers the area of bounding box bbox
    edge_grid(double w, double h, const util::geo::Box<PFAEDLE_PRECISION>& bbox);

    // initialization of a point grid with cell width w and cell height h
    // that covers the area of bounding box bbox
    // optional parameters specifies whether a value->cell index
    // should be kept (true by default!)
    edge_grid(double w, double h, const util::geo::Box<PFAEDLE_PRECISION>& bbox, bool buildValIdx);

    // the empty grid
    edge_grid();

    // the empty grid
    edge_grid(bool buildValIdx);

    edge_candidate_priority_queue get_edge_candidates(const POINT& s, double d);
};

}

#endif//CONFIG_FILE_PARSER_H_EDGE_GRID_H

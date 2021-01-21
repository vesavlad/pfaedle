#ifndef CONFIG_FILE_PARSER_H_NODE_GRID_H
#define CONFIG_FILE_PARSER_H_NODE_GRID_H

#include <pfaedle/trgraph/edge_payload.h>
#include <pfaedle/trgraph/node_payload.h>
#include <util/graph/Node.h>
#include <util/geo/Grid.h>

namespace pfaedle::trgraph
{
// graph class
class graph;

using node = util::graph::Node<node_payload, edge_payload>;

class node_grid : public util::geo::Grid<node*, util::geo::Point, PFAEDLE_PRECISION>
{
public:
    static trgraph::node_grid build_node_grid(graph& g, size_t size, const BOX& webMercBox, bool which);

    // initialization of a point grid with cell width w and cell height h
    // that covers the area of bounding box bbox
    node_grid(double w, double h, const util::geo::Box<PFAEDLE_PRECISION>& bbox);

    // initialization of a point grid with cell width w and cell height h
    // that covers the area of bounding box bbox
    // optional parameters specifies whether a value->cell index
    // should be kept (true by default!)
    node_grid(double w, double h, const util::geo::Box<PFAEDLE_PRECISION>& bbox, bool buildValIdx);

    // the empty grid
    node_grid();

    // the empty grid
    node_grid(bool buildValIdx);

    node* get_matching_node(const node_payload& s, double d);

    node* get_distance_matching_node(const trgraph::node_payload& s, double d);

    std::set<node*> get_matching_nodes(const node_payload& s, double d);
};


}

#endif//CONFIG_FILE_PARSER_H_NODE_GRID_H

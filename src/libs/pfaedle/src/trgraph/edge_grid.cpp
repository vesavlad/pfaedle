#include "pfaedle/trgraph/edge_grid.h"
#include "pfaedle/trgraph/graph.h"

namespace pfaedle::trgraph
{
edge_grid::edge_grid(double w, double h, const util::geo::Box<double>& bbox) :
    Grid(w, h, bbox)
{}
edge_grid::edge_grid(double w, double h, const util::geo::Box<double>& bbox, bool buildValIdx) :
    Grid(w, h, bbox, buildValIdx)
{
}
edge_grid::edge_grid() :
    Grid()
{
}
edge_grid::edge_grid(bool buildValIdx) :
    Grid(buildValIdx)
{
}
edge_grid::edge_candidate_priority_queue edge_grid::get_edge_candidates(const POINT& s, double d)
{
    edge_candidate_priority_queue ret;
    double distor = util::geo::webMercDistFactor(s);
    std::set<edge*> neighs;
    BOX box = util::geo::pad(util::geo::getBoundingBox(s), d / distor);
    get(box, &neighs);

    for (auto* e : neighs)
    {
        double dist = util::geo::distToSegment(*e->getFrom()->pl().get_geom(),
                                               *e->getTo()->pl().get_geom(), s);

        if (dist * distor <= d)
        {
            ret.emplace(-dist, e);
        }
    }

    return ret;
}
trgraph::edge_grid edge_grid::build_edge_grid(graph& g, size_t size, const BOX& webMercBox)
{
    trgraph::edge_grid ret(size, size, webMercBox, false);
    for (auto& n : g.getNds())
    {
        for (auto* e : n->getAdjListOut())
        {
            assert(e->pl().get_geom());
            ret.add(*e->pl().get_geom(), e);
        }
    }
    return ret;
}
}

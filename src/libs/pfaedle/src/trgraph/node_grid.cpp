#include "pfaedle/trgraph/node_grid.h"
#include "pfaedle/trgraph/graph.h"

namespace pfaedle::trgraph
{

node_grid::node_grid(double w, double h, const util::geo::Box<double>& bbox) :
    Grid(w, h, bbox)
{}
node_grid::node_grid(double w, double h, const util::geo::Box<double>& bbox, bool buildValIdx) :
    Grid(w, h, bbox, buildValIdx)
{
}
node_grid::node_grid() :
    Grid()
{
}
node_grid::node_grid(bool buildValIdx) :
    Grid(buildValIdx)
{
}
node* node_grid::get_matching_node(const node_payload& s, double d)
{
    double distor = util::geo::webMercDistFactor(*s.get_geom());
    std::set<node*> neighs;
    BOX box = util::geo::pad(util::geo::getBoundingBox(*s.get_geom()), d / distor);
    get(box, &neighs);

    node* ret = nullptr;
    double best_d = std::numeric_limits<double>::max();

    for (auto* n : neighs)
    {
        if (n->pl().get_si() && n->pl().get_si()->simi(s.get_si()) > 0.5)
        {
            double dist = webMercMeterDist(*n->pl().get_geom(), *s.get_geom());
            if (dist < d && dist < best_d)
            {
                best_d = dist;
                ret = n;
            }
        }
    }

    return ret;
}
node* node_grid::get_distance_matching_node(const node_payload& s, double d)
{
    double distor = util::geo::webMercDistFactor(*s.get_geom());
    std::set<node*> neighs;
    BOX box = util::geo::pad(util::geo::getBoundingBox(*s.get_geom()), d / distor);
    get(box, &neighs);

    node* ret = nullptr;
    double best_d = std::numeric_limits<double>::max();

    for (auto* n : neighs)
    {
        // name can be different therefore don't enfore name similarities
        if (n->pl().get_si())
        {
            double dist = webMercMeterDist(*n->pl().get_geom(), *s.get_geom());

            if (dist < d && dist < best_d)
            {
                best_d = dist;
                ret = n;
            }
        }
    }

    return ret;
}
std::set<node*> node_grid::get_matching_nodes(const node_payload& s, double d)
{
    std::set<node*> ret;
    double distor = util::geo::webMercDistFactor(*s.get_geom());
    std::set<node*> neighs;
    BOX box = util::geo::pad(util::geo::getBoundingBox(*s.get_geom()), d / distor);
    get(box, &neighs);

    for (auto* n : neighs)
    {
        if (n->pl().get_si() && n->pl().get_si()->simi(s.get_si()) > 0.5)
        {
            double dist = webMercMeterDist(*n->pl().get_geom(), *s.get_geom());
            if (dist < d) ret.insert(n);
        }
    }

    return ret;
}
trgraph::node_grid node_grid::build_node_grid(graph& g, size_t size, const BOX& webMercBox, bool which)
{
    trgraph::node_grid ret(size, size, webMercBox, false);
    for (auto* n : g.getNds())
    {
        if (!which && n->getInDeg() + n->getOutDeg() == 1)
            ret.add(*n->pl().get_geom(), n);
        else if (which && n->pl().get_si())
            ret.add(*n->pl().get_geom(), n);
    }
    return ret;
}
}
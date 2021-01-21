#include <pfaedle/trgraph/graph.h>
#include <util/geo/Geo.h>
namespace pfaedle::trgraph
{

template<typename F>
inline bool angSmaller(const util::geo::Point<F>& f,
                       const util::geo::Point<F>& m,
                       const util::geo::Point<F>& t,
                       double ang)
{
    if (util::geo::innerProd(m, f, t) < ang)
    {
        return true;
    }

    return false;
}

inline double web_mercator_distance(const node& a, const node& b)
{
    return webMercMeterDist(*a.pl().get_geom(), *b.pl().get_geom());
}


inline bool keep_full_turn(const trgraph::node& n, double ang)
{
    if (n.getInDeg() + n.getOutDeg() != 1) return false;

    const trgraph::edge* e = nullptr;
    if (n.getOutDeg())
        e = n.getAdjListOut().front();
    else
        e = n.getAdjListIn().front();

    auto other = e->getOtherNd(&n);

    if (other->getInDeg() + other->getOutDeg() == 3)
    {
        const trgraph::edge* a = nullptr;
        const trgraph::edge* b = nullptr;
        for (auto f : other->getAdjListIn())
        {
            if (f != e && !a)
                a = f;
            else if (f != e && !b)
                b = f;
        }

        for (auto f : other->getAdjListOut())
        {
            if (f != e && !a)
                a = f;
            else if (f != e && !b)
                b = f;
        }

        auto ap = a->pl().backHop();
        auto bp = b->pl().backHop();
        if (a->getTo() != other) ap = a->pl().frontHop();
        if (b->getTo() != other) bp = b->pl().frontHop();

        return angSmaller(ap, *other->pl().get_geom(), bp, ang);
    }

    return false;
}


void graph::write_geometries()
{
    for (auto& n : getNds())
    {
        for (auto* e : n->getAdjListOut())
        {
            auto distance = webMercMeterDist(*e->getFrom()->pl().get_geom(),
                                             *e->getTo()->pl().get_geom());
            e->pl().add_point(*e->getFrom()->pl().get_geom());
            e->pl().set_length(distance);
            e->pl().add_point(*e->getTo()->pl().get_geom());
        }
    }
}
void graph::delete_orphan_nodes()
{
    auto& nds = getNds();
    for (auto it = nds.begin(); it != nds.end();)
    {
        auto* node = (*it);

        if ((node->getInDeg() + node->getOutDeg()) == 0 &&
            !(node->pl().get_si() && node->pl().get_si()->get_group()))
        {
            it = delNd(it);
            // TODO(patrick): maybe delete from node grid?
        }
        else
        {
            it++;
        }
    }
}
void graph::collapse_edges()
{
    for (auto* n : getNds())
    {
        if (n->getOutDeg() + n->getInDeg() != 2 || n->pl().get_si()) continue;

        edge* ea;
        edge* eb;
        if (n->getOutDeg() == 2)
        {
            ea = *n->getAdjListOut().begin();
            eb = *n->getAdjListOut().rbegin();
        }
        else if (n->getInDeg() == 2)
        {
            ea = *n->getAdjListIn().begin();
            eb = *n->getAdjListIn().rbegin();
        }
        else
        {
            ea = *n->getAdjListOut().begin();
            eb = *n->getAdjListIn().begin();
        }

        // important, we don't have a multigraph! if the same edge
        // will already exist, leave this node
        if (getEdg(ea->getOtherNd(n), eb->getOtherNd(n))) continue;
        if (getEdg(eb->getOtherNd(n), ea->getOtherNd(n))) continue;

        if (are_edges_similar(*ea, *eb))
        {
            if (ea->pl().oneWay() && ea->getOtherNd(n) != ea->getFrom())
            {
                addEdg(eb->getOtherNd(n), ea->getOtherNd(n), merge_edge_payload(*eb, *ea));
            }
            else
            {
                addEdg(ea->getOtherNd(n), eb->getOtherNd(n), merge_edge_payload(*ea, *eb));
            }
            delEdg(ea->getFrom(), ea->getTo());
            delEdg(eb->getFrom(), eb->getTo());
        }
    }
}
void graph::simplify_geometries()
{
    for (auto* n : getNds())
    {
        for (auto* e : n->getAdjListOut())
        {
            (*e->pl().get_geom()) = util::geo::simplify(*e->pl().get_geom(), 0.5);
        }
    }
}
uint32_t graph::write_components()
{
    auto* comp = new component{7};
    uint32_t comp_counter = 0;

    for (auto* n : getNds())
    {
        if (!n->pl().get_component())
        {
            using node_edge_pair = std::pair<node*, edge*>;
            std::stack<node_edge_pair> stack({node_edge_pair(n, 0)});
            while (!stack.empty())
            {
                auto stack_top = stack.top();
                stack.pop();

                stack_top.first->pl().set_component(comp);
                for (auto* e : stack_top.first->getAdjListOut())
                {
                    if (e->pl().level() < comp->minEdgeLvl)
                        comp->minEdgeLvl = e->pl().level();
                    if (!e->getOtherNd(stack_top.first)->pl().get_component())
                        stack.emplace(e->getOtherNd(stack_top.first), e);
                }
                for (auto* e : stack_top.first->getAdjListIn())
                {
                    if (e->pl().level() < comp->minEdgeLvl)
                        comp->minEdgeLvl = e->pl().level();
                    if (!e->getOtherNd(stack_top.first)->pl().get_component())
                        stack.emplace(e->getOtherNd(stack_top.first), e);
                }
            }

            comp_counter++;
            comp = new component{7};
        }
    }

    // the last comp was not used
    delete comp;

    return comp_counter;
}
void graph::writeSelfEdgs()
{
    for (auto* n : getNds())
    {
        if (n->pl().get_si() && n->getAdjListOut().empty())
        {
            addEdg(n, n);
        }
    }
}
void graph::fix_gaps(trgraph::node_grid& ng)
{
    double meter = 1;
    for (auto& n : getNds())
    {
        if (n->getInDeg() + n->getOutDeg() == 1)
        {
            // get all nodes in distance
            std::set<node*> ret;
            double distor = util::geo::webMercDistFactor(*n->pl().get_geom());

            ng.get(util::geo::pad(util::geo::getBoundingBox(*n->pl().get_geom()),
                                   meter / distor),
                    &ret);
            for (auto* nb : ret)
            {
                if (nb != n && (nb->getInDeg() + nb->getOutDeg()) == 1 &&
                    web_mercator_distance(*nb, *n) <= meter / distor)
                {
                    // special case: both node are non-stations, move
                    // the end point nb to n and delete nb
                    if (!nb->pl().get_si() && !n->pl().get_si())
                    {
                        node* otherN;
                        if (nb->getOutDeg())
                            otherN = (*nb->getAdjListOut().begin())->getOtherNd(nb);
                        else
                            otherN = (*nb->getAdjListIn().begin())->getOtherNd(nb);
                        LINE l;
                        l.push_back(*otherN->pl().get_geom());
                        l.push_back(*n->pl().get_geom());

                        edge* e;
                        if (nb->getOutDeg())
                            e = addEdg(otherN, n, (*nb->getAdjListOut().begin())->pl());
                        else
                            e = addEdg(otherN, n, (*nb->getAdjListIn().begin())->pl());
                        if (e)
                        {
                            *e->pl().get_geom() = l;
                            delNd(nb);
                            ng.remove(nb);
                        }
                    }
                    else
                    {
                        // if one of the nodes is a station, just add an edge between them
                        if (nb->getOutDeg())
                            addEdg(n, nb, (*nb->getAdjListOut().begin())->pl());
                        else
                            addEdg(n, nb, (*nb->getAdjListIn().begin())->pl());
                    }
                }
            }
        }
    }
}
bool graph::are_edges_similar(const edge& a, const edge& b)
{
    if (static_cast<bool>(a.pl().oneWay()) ^ static_cast<bool>(b.pl().oneWay()))
        return false;
    if (a.pl().level() != b.pl().level())
        return false;
    if (a.pl().get_lines().size() != b.pl().get_lines().size())
        return false;
    if (a.pl().get_lines() != b.pl().get_lines())
        return false;

    if (a.pl().oneWay() && b.pl().oneWay())
    {
        if (a.getFrom() != b.getTo() && a.getTo() != b.getFrom())
            return false;
    }
    if (a.pl().is_restricted() || b.pl().is_restricted())
        return false;

    return true;
}
const edge_payload& graph::merge_edge_payload(edge& a, edge& b)
{
    const node* n = nullptr;
    if (a.getFrom() == b.getFrom())
        n = a.getFrom();
    else if (a.getFrom() == b.getTo())
        n = a.getFrom();
    else
        n = a.getTo();

    if (a.getTo() == n && b.getTo() == n)
    {
        // --> n <--
        a.pl().get_geom()->insert(a.pl().get_geom()->end(),
                                  b.pl().get_geom()->rbegin(),
                                  b.pl().get_geom()->rend());
    }
    else if (a.getTo() == n && b.getFrom() == n)
    {
        // --> n -->
        a.pl().get_geom()->insert(a.pl().get_geom()->end(),
                                  b.pl().get_geom()->begin(),
                                  b.pl().get_geom()->end());
    }
    else if (a.getFrom() == n && b.getTo() == n)
    {
        // <-- n <--
        std::reverse(a.pl().get_geom()->begin(), a.pl().get_geom()->end());
        a.pl().get_geom()->insert(a.pl().get_geom()->end(),
                                  b.pl().get_geom()->rbegin(),
                                  b.pl().get_geom()->rend());
    }
    else
    {
        // <-- n -->
        std::reverse(a.pl().get_geom()->begin(), a.pl().get_geom()->end());
        a.pl().get_geom()->insert(a.pl().get_geom()->end(),
                                  b.pl().get_geom()->begin(),
                                  b.pl().get_geom()->end());
    }

    a.pl().set_length(a.pl().get_length() + b.pl().get_length());

    return a.pl();
}
void graph::delete_orphan_edges(double turn_angle)
{
    const size_t rounds = 3;
    for (size_t c = 0; c < rounds; c++)
    {
        for (auto it = getNds().begin(); it != getNds().end();)
        {
            auto* node = (*it);
            if ((node->getInDeg() + node->getOutDeg()) != 1 || node->pl().get_si())
            {
                ++it;
                continue;
            }

            // check if the removal of this edge would transform a steep angle
            // full turn at an intersection into a node 2 eligible for contraction
            // if so, dont delete
            if (keep_full_turn(*node, turn_angle))
            {
                ++it;
                continue;
            }

            it = delNd(it);
            continue;
            it++;
        }
    }
}
}
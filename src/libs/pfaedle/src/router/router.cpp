// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_get_num_procs() 1
#endif

#include "pfaedle/router/router.h"
#include "pfaedle/router/routing_attributes.h"
#include "util/geo/output/GeoGraphJsonOutput.h"
#include "util/graph/Dijkstra.h"
#include "util/graph/EDijkstra.h"
#include <algorithm>
#include <logging/logger.h>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>


using util::graph::Dijkstra;
using util::graph::EDijkstra;

namespace pfaedle::router
{

edge_cost NCostFunc::operator()(const trgraph::node* from,
                               const trgraph::edge* e,
                               const trgraph::node* to) const
{
    UNUSED(to);
    if (!from) return edge_cost();

    int oneway = e->pl().oneWay() == 2;
    int32_t stationSkip = 0;

    return edge_cost(e->pl().lvl() == 0 ? e->pl().getLength() : 0,
                    e->pl().lvl() == 1 ? e->pl().getLength() : 0,
                    e->pl().lvl() == 2 ? e->pl().getLength() : 0,
                    e->pl().lvl() == 3 ? e->pl().getLength() : 0,
                    e->pl().lvl() == 4 ? e->pl().getLength() : 0,
                    e->pl().lvl() == 5 ? e->pl().getLength() : 0,
                    e->pl().lvl() == 6 ? e->pl().getLength() : 0,
                    e->pl().lvl() == 7 ? e->pl().getLength() : 0, 0, stationSkip,
                    e->pl().getLength() * oneway, oneway, 0, 0, 0, &_rOpts);
}

edge_cost CostFunc::operator()(const trgraph::edge* from, const trgraph::node* n,
                              const trgraph::edge* to) const
{
    if (!from) return edge_cost();

    uint32_t fullTurns = 0;
    int oneway = from->pl().oneWay() == 2;
    int32_t stationSkip = 0;

    if (n)
    {
        if (from->getFrom() == to->getTo() && from->getTo() == to->getFrom())
        {
            // trivial full turn
            fullTurns = 1;
        }
        else if (n->getDeg() > 2)
        {
            // otherwise, only intersection angles will be punished
            fullTurns = angSmaller(from->pl().backHop(), *n->pl().getGeom(),
                                           to->pl().frontHop(), _rOpts.fullTurnAngle);
        }

        if (from->pl().isRestricted() && !_res.may(from, to, n)) oneway = 1;

        // for debugging
        n->pl().setVisited();

        if (_tgGrp && n->pl().getSI() && n->pl().getSI()->getGroup() != _tgGrp)
            stationSkip = 1;
    }

    double transitLinePen = transitLineCmp(from->pl());
    bool noLines = (_rAttrs.short_name.empty() && _rAttrs.to.empty() &&
                    _rAttrs.from.empty() && from->pl().getLines().empty());

    return edge_cost(from->pl().lvl() == 0 ? from->pl().getLength() : 0,
                    from->pl().lvl() == 1 ? from->pl().getLength() : 0,
                    from->pl().lvl() == 2 ? from->pl().getLength() : 0,
                    from->pl().lvl() == 3 ? from->pl().getLength() : 0,
                    from->pl().lvl() == 4 ? from->pl().getLength() : 0,
                    from->pl().lvl() == 5 ? from->pl().getLength() : 0,
                    from->pl().lvl() == 6 ? from->pl().getLength() : 0,
                    from->pl().lvl() == 7 ? from->pl().getLength() : 0, fullTurns,
                    stationSkip, from->pl().getLength() * oneway, oneway,
                    from->pl().getLength() * transitLinePen,
                    noLines ? from->pl().getLength() : 0, 0, &_rOpts);
}

double CostFunc::transitLineCmp(const trgraph::edge_payload& e) const
{
    if (_rAttrs.short_name.empty() && _rAttrs.to.empty() &&
        _rAttrs.from.empty())
        return 0;
    double best = 1;
    for (const auto* l : e.getLines())
    {
        double cur = _rAttrs.simi(l);

        if (cur < 0.0001) return 0;
        if (cur < best) best = cur;
    }

    return best;
}

NDistHeur::NDistHeur(const routing_options& rOpts,
                     const std::set<trgraph::node*>& tos) :
    _rOpts(rOpts),
    _maxCentD(0)
{
    size_t c = 0;
    double x = 0, y = 0;
    for (auto to : tos)
    {
        x += to->pl().getGeom()->getX();
        y += to->pl().getGeom()->getY();
        c++;
    }

    x /= c;
    y /= c;
    _center = POINT(x, y);

    for (auto to : tos)
    {
        double cur = util::geo::webMercMeterDist(*to->pl().getGeom(), _center);
        if (cur > _maxCentD) _maxCentD = cur;
    }
}

DistHeur::DistHeur(uint8_t minLvl, const routing_options& rOpts,
                   const std::set<trgraph::edge*>& tos) :
    _rOpts(rOpts),
    _lvl(minLvl), _maxCentD(0)
{
    size_t c = 0;
    double x = 0, y = 0;
    for (auto to : tos)
    {
        x += to->getFrom()->pl().getGeom()->getX();
        y += to->getFrom()->pl().getGeom()->getY();
        c++;
    }

    x /= c;
    y /= c;
    _center = POINT(x, y);

    for (auto to : tos)
    {
        double cur = util::geo::webMercMeterDist(*to->getFrom()->pl().getGeom(), _center) *
                     _rOpts.levelPunish[_lvl];
        if (cur > _maxCentD) _maxCentD = cur;
    }
}

edge_cost DistHeur::operator()(const trgraph::edge* a,
                              const std::set<trgraph::edge*>& b) const
{
    UNUSED(b);
    double cur = util::geo::webMercMeterDist(*a->getFrom()->pl().getGeom(), _center) *
                 _rOpts.levelPunish[_lvl];

    return edge_cost(cur - _maxCentD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr);
}

edge_cost NDistHeur::operator()(const trgraph::node* a,
                               const std::set<trgraph::node*>& b) const
{
    UNUSED(b);
    double cur = util::geo::webMercMeterDist(*a->pl().getGeom(), _center);

    return edge_cost(cur - _maxCentD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr);
}

double CombCostFunc::operator()(const edge* from, const node* n,
                                const edge* to) const
{
    UNUSED(n);
    UNUSED(from);
    return to->pl().getCost().getValue();
}

router::router(size_t numThreads, bool caching) :
    _cache(numThreads),
    _caching(caching)
{
    for (size_t i = 0; i < numThreads; i++)
    {
        _cache[i] = new Cache();
    }
}

router::~router()
{
    for (auto & i : _cache)
    {
        delete i;
    }
}

bool router::compConned(const edge_candidate_group& a, const edge_candidate_group& b) const
{
    for (auto n1 : a)
    {
        for (auto n2 : b)
        {
            if (n1.e->getFrom()->pl().getComp() == n2.e->getFrom()->pl().getComp())
                return true;
        }
    }

    return false;
}

HopBand router::getHopBand(const edge_candidate_group& a, const edge_candidate_group& b,
                           const routing_attributes& rAttrs, const routing_options& rOpts,
                           const osm::Restrictor& rest) const
{
    assert(a.size());
    assert(b.size());

    double pend = 0;
    for (auto i : a)
    {
        for (auto j : b)
        {
            double d = util::geo::webMercMeterDist(*i.e->getFrom()->pl().getGeom(),
                                        *j.e->getFrom()->pl().getGeom());
            if (d > pend) pend = d;
        }
    }

    LOG(TRACE) << "Pending max hop distance is " << pend << " meters";

    const trgraph::station_group* tgGrpTo = nullptr;

    if (b.begin()->e->getFrom()->pl().getSI())
        tgGrpTo = b.begin()->e->getFrom()->pl().getSI()->getGroup();

    CostFunc costF(rAttrs, rOpts, rest, tgGrpTo, pend * 50);

    std::set<trgraph::edge*> from, to;

    for (auto e : a) from.insert(e.e);
    for (auto e : b) to.insert(e.e);

    LOG(TRACE) << "Doing pilot run between " << from.size() << "->" << to.size()
                << " edge candidates";

    edge_list el;
    edge_cost ret = costF.inf();
    DistHeur distH(0, rOpts, to);

    if (compConned(a, b))
        ret = EDijkstra::shortestPath(from, to, costF, distH, &el);

    if (el.size() < 2 && costF.inf() <= ret)
    {
        LOG(TRACE) << "Pilot run: no connection between candidate groups,"
                    << " setting max distance to 1";
        return HopBand{0, 1, nullptr, 0};
    }

    // cache the found path, will save a few dijkstra iterations
    nestedCache(&el, from, costF, rAttrs);

    auto na = el.back()->getFrom();
    auto nb = el.front()->getFrom();

    double maxStrD = 0;

    for (auto e : to)
    {
        double d = util::geo::webMercMeterDist(*el.front()->getFrom()->pl().getGeom(),
                                    *e->getTo()->pl().getGeom());
        if (d > maxStrD) maxStrD = d;
    }

    // TODO(patrick): derive the punish level here automatically
    double maxD = std::max(ret.getValue(), pend * rOpts.levelPunish[2]) * 3 +
                  rOpts.fullTurnPunishFac + rOpts.platformUnmatchedPen;
    double minD = ret.getValue();

    LOG(TRACE) << "Pilot run: min distance between two groups is "
                << ret.getValue() << " (between nodes " << na << " and " << nb
                << "), using a max routing distance of " << maxD << ". The max"
                << " straight line distance from the pilot target to any other "
                   "target node was"
                << " " << maxStrD << ".";

    return HopBand{minD, maxD, el.front(), maxStrD};
}

edge_list_hops router::routeGreedy(const node_candidate_route& route,
                                 const routing_attributes& rAttrs,
                                 const routing_options& rOpts,
                                 const osm::Restrictor& rest) const
{
    if (route.size() < 2)
        return edge_list_hops();
    edge_list_hops ret(route.size() - 1);

    for (size_t i = 0; i < route.size() - 1; i++)
    {
        const trgraph::station_group* tgGrp = nullptr;
        std::set<trgraph::node*> from, to;
        for (auto c : route[i]) from.insert(c.nd);
        for (auto c : route[i + 1]) to.insert(c.nd);
        if (route[i + 1].begin()->nd->pl().getSI())
            tgGrp = route[i + 1].begin()->nd->pl().getSI()->getGroup();

        NCostFunc cost(rAttrs, rOpts, rest, tgGrp);
        NDistHeur dist(rOpts, to);

        node_list nodesRet;
        edge_list_hop hop;
        Dijkstra::shortestPath(from, to, cost, dist, &hop.edges, &nodesRet);

        if (nodesRet.size() > 1)
        {
            // careful: nodesRet is reversed!
            hop.start = nodesRet.back();
            hop.end = nodesRet.front();
        }
        else
        {
            // just take the first candidate if no route could be found
            hop.start = *from.begin();
            hop.end = *to.begin();
        }

        ret[i] = hop;
    }

    return ret;
}

edge_list_hops router::routeGreedy2(const node_candidate_route& route,
                                  const routing_attributes& rAttrs,
                                  const routing_options& rOpts,
                                  const osm::Restrictor& rest) const
{
    if (route.size() < 2) return edge_list_hops();
    edge_list_hops ret(route.size() - 1);

    for (size_t i = 0; i < route.size() - 1; i++)
    {
        const trgraph::station_group* tgGrp = nullptr;
        std::set<trgraph::node*> from, to;

        if (i == 0)
            for (auto c : route[i]) from.insert(c.nd);
        else
            from.insert(const_cast<trgraph::node*>(ret[i - 1].end));

        for (auto c : route[i + 1]) to.insert(c.nd);

        if (route[i + 1].begin()->nd->pl().getSI())
            tgGrp = route[i + 1].begin()->nd->pl().getSI()->getGroup();

        NCostFunc cost(rAttrs, rOpts, rest, tgGrp);
        NDistHeur dist(rOpts, to);

        node_list nodesRet;
        edge_list_hop hop;
        Dijkstra::shortestPath(from, to, cost, dist, &hop.edges, &nodesRet);
        if (nodesRet.size() > 1)
        {
            // careful: nodesRet is reversed!
            hop.start = nodesRet.back();
            hop.end = nodesRet.front();
        }
        else
        {
            // just take the first candidate if no route could be found
            hop.start = *from.begin();
            hop.end = *to.begin();
        }

        ret[i] = hop;
    }

    return ret;
}

edge_list_hops router::route(const edge_candidate_route& route,
                           const routing_attributes& rAttrs, const routing_options& rOpts,
                           const osm::Restrictor& rest) const
{
    graph cg;
    return router::route(route, rAttrs, rOpts, rest, &cg);
}

edge_list_hops router::route(const edge_candidate_route& route,
                           const routing_attributes& rAttrs, const routing_options& rOpts,
                           const osm::Restrictor& rest,
                           graph* cgraph) const
{
    if (route.size() < 2)
        return edge_list_hops();
    edge_list_hops ret(route.size() - 1);

    CombCostFunc ccost(rOpts);
    node* source = cgraph->addNd();
    node* sink = cgraph->addNd();
    CombNodeMap nodes;
    CombNodeMap nextNodes;

    for (auto i : route[0])
    {
        auto e = i.e;
        // we can be sure that each edge is exactly assigned to only one
        // node because the transitgraph is directed
        nodes[e] = cgraph->addNd(i.e->getFrom());
        cgraph->addEdg(source, nodes[e])
                ->pl()
                .setCost(edge_cost(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  i.pen, nullptr));
    }

    size_t iters = EDijkstra::ITERS;
    double itPerSecTot = 0;
    size_t n = 0;
    for (size_t i = 0; i < route.size() - 1; i++)
    {
        nextNodes.clear();
        HopBand hopBand = getHopBand(route[i], route[i + 1], rAttrs, rOpts, rest);

        const trgraph::station_group* tgGrp = nullptr;
        if (route[i + 1].begin()->e->getFrom()->pl().getSI())
            tgGrp = route[i + 1].begin()->e->getFrom()->pl().getSI()->getGroup();

        std::set<trgraph::edge*> froms;
        for (const auto& fr : route[i]) froms.insert(fr.e);

        for (auto eFr : froms)
        {
            node* cNodeFr = nodes.find(eFr)->second;

            edge_set tos;
            std::map<trgraph::edge*, edge*> edges;
            std::map<trgraph::edge*, double> pens;
            std::unordered_map<trgraph::edge*, edge_list*> edgeLists;
            std::unordered_map<trgraph::edge*, edge_cost> costs;

            assert(route[i + 1].size());

            for (const auto& to : route[i + 1])
            {
                auto eTo = to.e;
                tos.insert(eTo);

                if (!nextNodes.count(eTo))
                    nextNodes[eTo] = cgraph->addNd(to.e->getFrom());

                if (i == route.size() - 2)
                    cgraph->addEdg(nextNodes[eTo], sink);

                edges[eTo] = cgraph->addEdg(cNodeFr, nextNodes[eTo]);
                pens[eTo] = to.pen;

                edgeLists[eTo] = edges[eTo]->pl().getEdges();
                edges[eTo]->pl().setStartNode(eFr->getFrom());

                // for debugging
                edges[eTo]->pl().setStartEdge(eFr);
                edges[eTo]->pl().setEndNode(to.e->getFrom());

                // for debugging
                edges[eTo]->pl().setEndEdge(eTo);
            }

            size_t iters = EDijkstra::ITERS;
            auto t1 = TIME();

            assert(tos.size());
            assert(froms.size());

            hops(eFr, froms, tos, tgGrp, edgeLists, &costs, rAttrs, rOpts, rest, hopBand);
            double itPerSec = (static_cast<double>(EDijkstra::ITERS - iters)) / TOOK(t1, TIME());
            n++;
            itPerSecTot += itPerSec;

            LOG(TRACE) << "from " << eFr << ": 1-" << tos.size() << " ("
                        << route[i + 1].size() << " nodes) hop took "
                        << EDijkstra::ITERS - iters << " iterations, "
                        << TOOK(t1, TIME()) << "ms (tput: " << itPerSec << " its/ms)";
            for (auto& kv : edges)
            {
                kv.second->pl().setCost(edge_cost(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, pens[kv.first], nullptr) + costs[kv.first]);

                if (rOpts.popReachEdge && !kv.second->pl().getEdges()->empty())
                {
                    if (kv.second->pl().getEdges() &&
                        !kv.second->pl().getEdges()->empty())
                    {
                        // the reach edge is included, but we dont want it in the geometry
                        kv.second->pl().getEdges()->erase(kv.second->pl().getEdges()->begin());
                    }
                }
            }
        }

        std::swap(nodes, nextNodes);
    }

    LOG(TRACE) << "Hops took " << EDijkstra::ITERS - iters << " iterations,"
                << " average tput was " << (itPerSecTot / n) << " its/ms";

    iters = EDijkstra::ITERS;
    std::vector<edge*> res;
    EDijkstra::shortestPath(source, sink, ccost, &res);
    size_t j = 0;

    LOG(TRACE) << "Optim graph solve took " << EDijkstra::ITERS - iters << " iterations.";

    for (auto i = res.rbegin(); i != res.rend(); i++)
    {
        const auto e = *i;
        if (e->getFrom() != source && e->getTo() != sink)
        {
            assert(e->pl().frontNode());
            assert(e->pl().backNode());

            ret[j] = edge_list_hop{std::move(*e->pl().getEdges()), e->pl().frontNode(),
                                 e->pl().backNode()};
            j++;
        }
    }

    assert(ret.size() == j);
    return ret;
}

edge_list_hops router::route(const node_candidate_route& route,
                           const routing_attributes& rAttrs, const routing_options& rOpts,
                           const osm::Restrictor& rest) const
{
    graph cg;
    return router::route(route, rAttrs, rOpts, rest, &cg);
}

edge_list_hops router::route(const node_candidate_route& route,
                           const routing_attributes& rAttrs, const routing_options& rOpts,
                           const osm::Restrictor& rest,
                           graph* cgraph) const
{
    edge_candidate_route r;
    for (auto& nCands : route)
    {
        r.emplace_back();
        for (auto n : nCands)
        {
            for (auto* e : n.nd->getAdjListOut())
            {
                r.back().push_back(edge_candidate{e, n.pen});
            }
        }
    }

    return router::route(r, rAttrs, rOpts, rest, cgraph);
}

void router::hops(trgraph::edge* from, const std::set<trgraph::edge*>& froms,
                  const std::set<trgraph::edge*> tos,
                  const trgraph::station_group* tgGrp,
                  const std::unordered_map<trgraph::edge*, edge_list*>& edgesRet,
                  std::unordered_map<trgraph::edge*, edge_cost>* rCosts,
                  const routing_attributes& rAttrs, const routing_options& rOpts,
                  const osm::Restrictor& rest, HopBand hopB) const
{
    std::set<trgraph::edge*> rem;

    CostFunc cost(rAttrs, rOpts, rest, tgGrp, hopB.maxD);

    const auto& cached = getCachedHops(from, tos, edgesRet, rCosts, rAttrs);

    for (auto e : cached)
    {
        // shortcut: if the nodes lie in two different connected components,
        // the distance between them is trivially infinite
        if ((rOpts.noSelfHops && (e == from || e->getFrom() == from->getFrom())) ||
            from->getFrom()->pl().getComp() != e->getTo()->pl().getComp() ||
            e->pl().oneWay() == 2 || from->pl().oneWay() == 2)
        {
            (*rCosts)[e] = cost.inf();
        }
        else
        {
            rem.insert(e);
        }
    }

    LOG(TRACE) << "From cache: " << tos.size() - rem.size()
                << ", have to cal: " << rem.size();

    if (!rem.empty())
    {
        DistHeur dist(from->getFrom()->pl().getComp()->minEdgeLvl, rOpts, rem);
        const auto& ret = EDijkstra::shortestPath(from, rem, cost, dist, edgesRet);
        for (const auto& kv : ret)
        {
            nestedCache(edgesRet.at(kv.first), froms, cost, rAttrs);

            (*rCosts)[kv.first] = kv.second;
        }
    }
}

void router::nestedCache(const edge_list* el,
                         const std::set<trgraph::edge*>& froms,
                         const CostFunc& cost,
                         const routing_attributes& rAttrs) const
{
    if (!_caching) return;
    if (el->empty()) return;
    // iterate over result edges backwards
    edge_list curEdges;
    edge_cost curCost;

    size_t j = 0;

    for (auto i = el->begin(); i < el->end(); i++)
    {
        if (!curEdges.empty())
        {
            curCost = curCost + cost(*i, (*i)->getTo(), curEdges.back());
        }

        curEdges.push_back(*i);

        if (froms.count(*i))
        {
            edge_cost startC = cost(nullptr, nullptr, *i) + curCost;
            cache(*i, el->front(), startC, &curEdges, rAttrs);
            j++;
        }
    }
}

std::set<pfaedle::trgraph::edge*> router::getCachedHops(
        trgraph::edge* from, const std::set<trgraph::edge*>& tos,
        const std::unordered_map<trgraph::edge*, edge_list*>& edgesRet,
        std::unordered_map<trgraph::edge*, edge_cost>* rCosts,
        const routing_attributes& rAttrs) const
{
    std::set<trgraph::edge*> ret;
    for (auto to : tos)
    {
        if (_caching && (*_cache[omp_get_thread_num()])[rAttrs][from].count(to))
        {
            const auto& cv = (*_cache[omp_get_thread_num()])[rAttrs][from][to];
            (*rCosts)[to] = cv.first;
            *edgesRet.at(to) = cv.second;
        }
        else
        {
            ret.insert(to);
        }
    }

    return ret;
}

void router::cache(trgraph::edge* from, trgraph::edge* to, const edge_cost& c,
                   edge_list* edges, const routing_attributes& rAttrs) const
{
    if (!_caching) return;
    if (from == to) return;
    (*_cache[omp_get_thread_num()])[rAttrs][from][to] =
            std::pair<edge_cost, edge_list>(c, *edges);
}

size_t router::getCacheNumber() const { return _cache.size(); }

}

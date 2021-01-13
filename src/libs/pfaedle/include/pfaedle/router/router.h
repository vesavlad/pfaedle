// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_ROUTER_H_
#define PFAEDLE_ROUTER_ROUTER_H_

#include "pfaedle/definitions.h"
#include "pfaedle/osm/Restrictor.h"
#include "pfaedle/router/graph.h"
#include "pfaedle/router/misc.h"
#include "pfaedle/router/routing_attributes.h"
#include "pfaedle/trgraph/graph.h"
#include "util/geo/Geo.h"
#include "util/graph/Dijkstra.h"
#include "util/graph/EDijkstra.h"

#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pfaedle::router
{
using CombNodeMap = std::unordered_map<const trgraph::edge*, router::node*>;
using HId = std::pair<size_t, size_t>;
using Cache = std::map<
            routing_attributes,
            std::unordered_map<const trgraph::edge*,std::unordered_map<const trgraph::edge*, std::pair<edge_cost, edge_list>>>
        >;

struct HopBand
{
    double minD;
    double maxD;
    const trgraph::edge* nearest;
    double maxInGrpDist;
};

struct CostFunc : public util::graph::EDijkstra::CostFunc<trgraph::node_payload, trgraph::edge_payload, edge_cost>
{
    CostFunc(const routing_attributes& rAttrs, const routing_options& rOpts,
             const osm::Restrictor& res, const trgraph::station_group* tgGrp,
             double max) :
        _rAttrs(rAttrs),
        _rOpts(rOpts),
        _res(res),
        _tgGrp(tgGrp),
        _inf(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, max, nullptr) {}

    const routing_attributes& _rAttrs;
    const routing_options& _rOpts;
    const osm::Restrictor& _res;
    const trgraph::station_group* _tgGrp;
    edge_cost _inf;

    edge_cost operator()(const trgraph::edge* from, const trgraph::node* n,
                        const trgraph::edge* to) const override;
    edge_cost inf() const override { return _inf; }

    double transitLineCmp(const trgraph::edge_payload& e) const;
};

struct NCostFunc : public util::graph::Dijkstra::CostFunc<trgraph::node_payload, trgraph::edge_payload, edge_cost>
{
    NCostFunc(const routing_attributes& rAttrs, const routing_options& rOpts,
              const osm::Restrictor& res, const trgraph::station_group* tgGrp) :
        _rAttrs(rAttrs),
        _rOpts(rOpts),
        _res(res),
        _tgGrp(tgGrp),
        _inf(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             std::numeric_limits<double>::infinity(), nullptr) {}

    const routing_attributes& _rAttrs;
    const routing_options& _rOpts;
    const osm::Restrictor& _res;
    const trgraph::station_group* _tgGrp;
    edge_cost _inf;

    edge_cost operator()(const trgraph::node* from, const trgraph::edge* e,
                        const trgraph::node* to) const override;
    edge_cost inf() const override { return _inf; }
};

struct DistHeur : public util::graph::EDijkstra::HeurFunc<trgraph::node_payload, trgraph::edge_payload, edge_cost>
{
    DistHeur(uint8_t minLvl, const routing_options& rOpts,
             const std::set<trgraph::edge*>& tos);

    const routing_options& _rOpts;
    uint8_t _lvl;
    POINT _center;
    double _maxCentD;
    edge_cost operator()(const trgraph::edge* a,
                        const std::set<trgraph::edge*>& b) const override;
};

struct NDistHeur : public util::graph::Dijkstra::HeurFunc<trgraph::node_payload, trgraph::edge_payload, edge_cost>
{
    NDistHeur(const routing_options& rOpts, const std::set<trgraph::node*>& tos);

    const routing_options& _rOpts;
    POINT _center;
    double _maxCentD;
    edge_cost operator()(const trgraph::node* a,
                        const std::set<trgraph::node*>& b) const override;
};

struct CombCostFunc : public util::graph::EDijkstra::CostFunc<router::node_payload, router::edge_payload, double>
{
    explicit CombCostFunc(const routing_options& rOpts) :
        _rOpts(rOpts) {}

    const routing_options& _rOpts;

    double operator()(const router::edge* from, const router::node* n,
                      const router::edge* to) const override;
    double inf() const override { return std::numeric_limits<double>::infinity(); }
};

/*
 * Finds the most likely route of schedule-based vehicle between stops in a
 * physical transportation network
 */
class router
{
public:
    // Init this router with caches for numThreads threads
    explicit router(size_t numThreads, bool caching);
    ~router();

    // Find the most likely path through the graph for a node candidate route.
    edge_list_hops route(const node_candidate_route& route, const routing_attributes& rAttrs,
                       const routing_options& rOpts,
                       const osm::Restrictor& rest) const;
    edge_list_hops route(const node_candidate_route& route, const routing_attributes& rAttrs,
                       const routing_options& rOpts, const osm::Restrictor& rest,
                       graph* cgraph) const;

    // Find the most likely path through the graph for an edge candidate route.
    edge_list_hops route(const edge_candidate_route& route, const routing_attributes& rAttrs,
                       const routing_options& rOpts,
                       const osm::Restrictor& rest) const;
    edge_list_hops route(const edge_candidate_route& route, const routing_attributes& rAttrs,
                       const routing_options& rOpts, const osm::Restrictor& rest,
                       graph* cgraph) const;

    // Find the most likely path through cgraph for a node candidate route, but
    // based on a greedy node to node approach
    edge_list_hops routeGreedy(const node_candidate_route& route,
                             const routing_attributes& rAttrs, const routing_options& rOpts,
                             const osm::Restrictor& rest) const;

    // Find the most likely path through cgraph for a node candidate route, but
    // based on a greedy node to node set approach
    edge_list_hops routeGreedy2(const node_candidate_route& route,
                              const routing_attributes& rAttrs,
                              const routing_options& rOpts,
                              const osm::Restrictor& rest) const;

    // Return the number of thread caches this router was initialized with
    size_t getCacheNumber() const;

private:
    mutable std::vector<Cache*> _cache;
    bool _caching;

    HopBand getHopBand(const edge_candidate_group& a, const edge_candidate_group& b,
                       const routing_attributes& rAttrs, const routing_options& rOpts,
                       const osm::Restrictor& rest) const;

    void hops(trgraph::edge* from, const std::set<trgraph::edge*>& froms,
              const std::set<trgraph::edge*> to, const trgraph::station_group* tgGrp,
              const std::unordered_map<trgraph::edge*, edge_list*>& edgesRet,
              std::unordered_map<trgraph::edge*, edge_cost>* rCosts,
              const routing_attributes& rAttrs, const routing_options& rOpts,
              const osm::Restrictor& rest, HopBand hopB) const;

    std::set<trgraph::edge*> getCachedHops(
            trgraph::edge* from, const std::set<trgraph::edge*>& to,
            const std::unordered_map<trgraph::edge*, edge_list*>& edgesRet,
            std::unordered_map<trgraph::edge*, edge_cost>* rCosts,
            const routing_attributes& rAttrs) const;

    void cache(trgraph::edge* from, trgraph::edge* to, const edge_cost& c,
               edge_list* edges, const routing_attributes& rAttrs) const;

    void nestedCache(const edge_list* el, const std::set<trgraph::edge*>& froms,
                     const CostFunc& cost, const routing_attributes& rAttrs) const;

    bool compConned(const edge_candidate_group& a, const edge_candidate_group& b) const;
};
}  // namespace pfaedle

#endif  // PFAEDLE_ROUTER_ROUTER_H_

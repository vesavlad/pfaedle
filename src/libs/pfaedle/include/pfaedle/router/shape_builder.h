// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_SHAPEBUILDER_H_
#define PFAEDLE_ROUTER_SHAPEBUILDER_H_

#include <pfaedle/config/config.h>
#include <pfaedle/config/mot_config.h>
#include <pfaedle/definitions.h>
#include <pfaedle/eval/collector.h>
#include <pfaedle/netgraph/graph.h>
#include <pfaedle/router/misc.h>
#include <pfaedle/router/router.h>
#include <pfaedle/trgraph/graph.h>
#include <pfaedle/trgraph/restrictor.h>

#include <gtfs/trip.h>
#include <util/geo/Geo.h>

#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace pfaedle::gtfs
{
class feed;
struct stop;
struct trip;
}

namespace pfaedle::router
{

struct shape
{
    edge_list_hops hops;
    double avgHopDist;
};

using cluster = std::vector<pfaedle::gtfs::trip*>;
using clusters = std::vector<cluster>;
using stop_pair = std::pair<const pfaedle::gtfs::stop*, const pfaedle::gtfs::stop*>;
using trip_routing_attributes = std::unordered_map<const pfaedle::gtfs::trip*, routing_attributes>;
using transit_graph_edges = std::unordered_map<const trgraph::edge*, std::set<const pfaedle::gtfs::trip*>>;

/*
 * Layer class for the router. Provides an interface for direct usage with
 * GTFS data
 */
class shape_builder
{
public:
    shape_builder(pfaedle::gtfs::feed& feed,
                  pfaedle::gtfs::feed& evalFeed,
                  route_type_set mots,
                  const config::mot_config& motCfg,
                  eval::collector& collector,
                  trgraph::graph& graph,
                  feed_stops& stops,
                  trgraph::restrictor& restr,
                  const config::config& cfg);

    void get_shape(pfaedle::netgraph::graph& ng);

    const node_candidate_group& get_node_candidates(const pfaedle::gtfs::stop& s) const;

    LINE get_shape_line(const node_candidate_route& ncr, const routing_attributes& rAttrs);
    LINE get_shape_line(pfaedle::gtfs::trip& trip);

    pfaedle::router::shape get_shape(pfaedle::gtfs::trip& trip) const;
    pfaedle::router::shape get_shape(pfaedle::gtfs::trip& trip);

    const trgraph::graph& get_graph() const;

    static void get_gtfs_box(const pfaedle::gtfs::feed& feed,
                      const route_type_set& mots,
                      const std::string& tid,
                      bool dropShapes,
                      osm::bounding_box& box);

private:
    clusters cluster_trips(pfaedle::gtfs::feed& f, const route_type_set& mots);
    void write_transit_graph(const pfaedle::router::shape& shp,
                             transit_graph_edges& edgs,
                             const cluster& cluster) const;
    void build_transit_graph(transit_graph_edges& edgs, pfaedle::netgraph::graph& ng) const;

    std::string get_free_shapeId(pfaedle::gtfs::trip& t);

    pfaedle::gtfs::shape get_gtfs_shape(const pfaedle::router::shape& shp,
                                            pfaedle::gtfs::trip& t,
                                            std::vector<double>& hopDists,
                                            std::vector<double>& hopTimes,
                                            std::vector<double>& costs);

    void set_shape(pfaedle::gtfs::trip* t,
                   const pfaedle::gtfs::shape& s,
                   const std::vector<double>& dists,
                   const std::vector<double>& costs);

    node_candidate_route get_node_candidate_route(pfaedle::gtfs::trip& trip) const;
    double get_average_hop_distance(pfaedle::gtfs::trip& trip) const;

    const routing_attributes& getRAttrs(const pfaedle::gtfs::trip& trip) const;
    const routing_attributes& getRAttrs(const pfaedle::gtfs::trip& trip);

    bool routingEqual(pfaedle::gtfs::trip& a,
                      pfaedle::gtfs::trip& b);
    bool routingEqual(const pfaedle::gtfs::stop& a,
                      const pfaedle::gtfs::stop& b);

    edge_list_hops route(const node_candidate_route& ncr,
                         const routing_attributes& rAttrs) const;

private:
    pfaedle::gtfs::feed& _feed;
    pfaedle::gtfs::feed& _evalFeed;
    route_type_set _mots;
    const config::mot_config& _motCfg;
    eval::collector& _ecoll;
    const config::config& _cfg;
    trgraph::graph& _g;
    router _crouter;

    feed_stops& _stops;

    node_candidate_group _emptyNCG;

    size_t _curShpCnt;
    size_t _numThreads;

    std::mutex _shpMutex;

    trip_routing_attributes _rAttrs;

    trgraph::restrictor& _restr;
};
}  // namespace pfaedle::router

#endif  // PFAEDLE_ROUTER_SHAPEBUILDER_H_

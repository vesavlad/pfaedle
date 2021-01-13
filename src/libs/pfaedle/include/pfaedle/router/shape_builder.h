// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_SHAPEBUILDER_H_
#define PFAEDLE_ROUTER_SHAPEBUILDER_H_

#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/config/config.h"
#include "pfaedle/config/mot_config.h"
#include "pfaedle/definitions.h"
#include "pfaedle/eval/collector.h"
#include "pfaedle/gtfs/Feed.h"
#include "pfaedle/netgraph/graph.h"
#include "pfaedle/osm/restrictor.h"
#include "pfaedle/router/misc.h"
#include "pfaedle/router/router.h"
#include "pfaedle/trgraph/graph.h"
#include "util/geo/Geo.h"

#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pfaedle::router
{

struct shape
{
    edge_list_hops hops;
    double avgHopDist;
};

using cluster = std::vector<pfaedle::gtfs::Trip*>;
using clusters = std::vector<cluster>;
using stop_pair = std::pair<const ad::cppgtfs::gtfs::Stop*, const ad::cppgtfs::gtfs::Stop*>;
using trip_routing_attributes = std::unordered_map<const pfaedle::gtfs::Trip*, routing_attributes>;
using transit_graph_edges = std::unordered_map<const trgraph::edge*, std::set<const pfaedle::gtfs::Trip*>>;

/*
 * Layer class for the router. Provides an interface for direct usage with
 * GTFS data
 */
class shape_builder
{
public:
    shape_builder(pfaedle::gtfs::Feed& feed,
                  ad::cppgtfs::gtfs::Feed& evalFeed,
                  MOTs mots,
                  const config::mot_config& motCfg,
                  eval::collector& ecoll,
                  trgraph::graph& g,
                  feed_stops& stops,
                  osm::restrictor& restr,
                  const config::config& cfg);

    void get_shape(pfaedle::netgraph::graph& ng);

    const node_candidate_group& getNodeCands(const ad::cppgtfs::gtfs::Stop& s) const;

    LINE get_shape_line(const node_candidate_route& ncr, const routing_attributes& rAttrs);
    LINE get_shape_line(pfaedle::gtfs::Trip& trip);

    pfaedle::router::shape get_shape(pfaedle::gtfs::Trip& trip) const;
    pfaedle::router::shape get_shape(pfaedle::gtfs::Trip& trip);

    const trgraph::graph& get_graph() const;

    static void get_gtfs_box(const pfaedle::gtfs::Feed& feed,
                             const MOTs& mots,
                             const std::string& tid,
                             bool dropShapes,
                             osm::bounding_box& box);

private:
    clusters cluster_trips(pfaedle::gtfs::Feed& f, const MOTs& mots);
    void write_transit_graph(const pfaedle::router::shape& shp,
                             transit_graph_edges& edgs,
                             const cluster& cluster) const;
    void build_transit_graph(transit_graph_edges& edgs, pfaedle::netgraph::graph& ng) const;

    std::string get_free_shapeId(Trip& t);

    ad::cppgtfs::gtfs::Shape get_gtfs_shape(const pfaedle::router::shape& shp,
                                            pfaedle::gtfs::Trip& t,
                                            std::vector<double>& hopDists);

    void set_shape(pfaedle::gtfs::Trip& t,
                   const ad::cppgtfs::gtfs::Shape& s,
                   const std::vector<double>& dists);

    node_candidate_route get_node_candidate_route(pfaedle::gtfs::Trip& trip) const;
    double get_average_hop_distance(pfaedle::gtfs::Trip& trip) const;

    const routing_attributes& getRAttrs(const pfaedle::gtfs::Trip& trip) const;
    const routing_attributes& getRAttrs(const pfaedle::gtfs::Trip& trip);

    bool routingEqual(pfaedle::gtfs::Trip& a,
                      pfaedle::gtfs::Trip& b);
    bool routingEqual(const ad::cppgtfs::gtfs::Stop& a,
                      const ad::cppgtfs::gtfs::Stop& b);

    edge_list_hops route(const node_candidate_route& ncr,
                         const routing_attributes& rAttrs) const;

private:
    pfaedle::gtfs::Feed& _feed;
    ad::cppgtfs::gtfs::Feed& _evalFeed;
    MOTs _mots;
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

    osm::restrictor& _restr;
};
}  // namespace pfaedle::router

#endif  // PFAEDLE_ROUTER_SHAPEBUILDER_H_

// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_SHAPEBUILDER_H_
#define PFAEDLE_ROUTER_SHAPEBUILDER_H_

#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/Def.h"
#include "pfaedle/config/MotConfig.h"
#include "pfaedle/config/PfaedleConfig.h"
#include "pfaedle/eval/Collector.h"
#include "pfaedle/gtfs/Feed.h"
#include "pfaedle/netgraph/Graph.h"
#include "pfaedle/osm/Restrictor.h"
#include "pfaedle/router/Misc.h"
#include "pfaedle/router/Router.h"
#include "pfaedle/trgraph/Graph.h"
#include "util/geo/Geo.h"

namespace pfaedle::router
{

struct Shape
{
    router::EdgeListHops hops;
    double avgHopDist;
};

using Cluster = std::vector<pfaedle::gtfs::Trip*>;
using Clusters = std::vector<Cluster>;
using StopPair = std::pair<const Stop*, const Stop*>;
using TripRAttrs = std::unordered_map<const pfaedle::gtfs::Trip*, router::RoutingAttrs>;
using TrGraphEdgs = std::unordered_map<const trgraph::Edge*, std::set<const pfaedle::gtfs::Trip*>>;

/*
 * Layer class for the router. Provides an interface for direct usage with
 * GTFS data
 */
class ShapeBuilder
{
public:
    ShapeBuilder(pfaedle::gtfs::Feed& feed,
                 ad::cppgtfs::gtfs::Feed& evalFeed,
                 MOTs mots,
                 const config::MotConfig& motCfg,
                 eval::Collector& ecoll,
                 trgraph::Graph& g,
                 router::FeedStops& stops,
                 osm::Restrictor& restr,
                 const config::Config& cfg);

    void shape(pfaedle::netgraph::Graph& ng);

    const NodeCandGroup& getNodeCands(const ad::cppgtfs::gtfs::Stop& s) const;

    LINE shapeL(const router::NodeCandRoute& ncr,
                const router::RoutingAttrs& rAttrs);
    LINE shapeL(pfaedle::gtfs::Trip& trip);

    pfaedle::router::Shape shape(pfaedle::gtfs::Trip& trip) const;
    pfaedle::router::Shape shape(pfaedle::gtfs::Trip& trip);

    const trgraph::Graph& getGraph() const;

    static void getGtfsBox(const pfaedle::gtfs::Feed& feed,
                           const MOTs& mots,
                           const std::string& tid,
                           bool dropShapes,
                           osm::BBoxIdx& box);

private:

    Clusters clusterTrips(pfaedle::gtfs::Feed& f, const MOTs& mots);
    void writeTransitGraph(const Shape& shp,
                           TrGraphEdgs& edgs,
                           const Cluster& cluster) const;
    void buildTrGraph(TrGraphEdgs& edgs, pfaedle::netgraph::Graph& ng) const;

    std::string getFreeShapeId(Trip& t);

    ad::cppgtfs::gtfs::Shape getGtfsShape(const Shape& shp,
                                          pfaedle::gtfs::Trip& t,
                                          std::vector<double>& hopDists);

    void setShape(pfaedle::gtfs::Trip& t,
                  const ad::cppgtfs::gtfs::Shape& s,
                  const std::vector<double>& dists);

    router::NodeCandRoute getNCR(pfaedle::gtfs::Trip& trip) const;
    double avgHopDist(pfaedle::gtfs::Trip& trip) const;

    const router::RoutingAttrs& getRAttrs(const pfaedle::gtfs::Trip& trip) const;
    const router::RoutingAttrs& getRAttrs(const pfaedle::gtfs::Trip& trip);

    bool routingEqual(pfaedle::gtfs::Trip& a,
                      pfaedle::gtfs::Trip& b);
    bool routingEqual(const ad::cppgtfs::gtfs::Stop& a,
                      const ad::cppgtfs::gtfs::Stop& b);

    router::EdgeListHops route(const router::NodeCandRoute& ncr,
                               const router::RoutingAttrs& rAttrs) const;

private:

    pfaedle::gtfs::Feed& _feed;
    ad::cppgtfs::gtfs::Feed& _evalFeed;
    MOTs _mots;
    const config::MotConfig& _motCfg;
    eval::Collector& _ecoll;
    const config::Config& _cfg;
    trgraph::Graph& _g;
    router::Router _crouter;

    router::FeedStops& _stops;

    NodeCandGroup _emptyNCG;

    size_t _curShpCnt;
    size_t _numThreads;

    std::mutex _shpMutex;

    TripRAttrs _rAttrs;

    osm::Restrictor& _restr;

};
}  // namespace pfaedle::router

#endif  // PFAEDLE_ROUTER_SHAPEBUILDER_H_

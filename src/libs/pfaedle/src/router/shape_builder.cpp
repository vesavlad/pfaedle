// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_get_num_procs() 1
#endif

#include "cppgtfs/gtfs/Feed.h"
#include "pfaedle/definitions.h"
#include "pfaedle/eval/collector.h"
#include "pfaedle/gtfs/Feed.h"
#include "pfaedle/gtfs/StopTime.h"
#include "pfaedle/osm/OsmBuilder.h"
#include "pfaedle/router/shape_builder.h"
#include "pfaedle/trgraph/station_group.h"
#include "util/geo/Geo.h"
#include "util/geo/output/GeoGraphJsonOutput.h"
#include "util/geo/output/GeoJsonOutput.h"
#include "util/graph/EDijkstra.h"
#include <exception>
#include <logging/logger.h>
#include <map>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>

using util::geo::DBox;
using util::geo::DPoint;
using util::geo::extendBox;

using ad::cppgtfs::gtfs::ShapePoint;
using ad::cppgtfs::gtfs::Stop;
using pfaedle::gtfs::Feed;
using pfaedle::gtfs::StopTime;
using pfaedle::gtfs::Trip;
using pfaedle::osm::BBoxIdx;
using util::geo::latLngToWebMerc;
using util::geo::webMercMeterDist;
using util::geo::webMercToLatLng;
using util::geo::output::GeoGraphJsonOutput;
using util::graph::Dijkstra;
using util::graph::EDijkstra;

namespace pfaedle::router
{
shape_builder::shape_builder(pfaedle::gtfs::Feed& feed,
                           ad::cppgtfs::gtfs::Feed& evalFeed,
                           MOTs mots,
                           const config::mot_config& motCfg,
                           eval::collector& ecoll,
                           trgraph::graph& g,
                           feed_stops& stops,
                           osm::Restrictor& restr,
                           const config::config& cfg) :
    _feed(feed),
    _evalFeed(evalFeed),
    _mots(std::move(mots)),
    _motCfg(motCfg),
    _ecoll(ecoll),
    _cfg(cfg),
    _g(g),
    _crouter(std::thread::hardware_concurrency(), cfg.useCaching),
    _stops(stops),
    _curShpCnt(0),
    _numThreads{_crouter.getCacheNumber()},
    _restr(restr)
{
}

const node_candidate_group& shape_builder::getNodeCands(const Stop& s) const
{
    if (_stops.find(&s) == _stops.end() || _stops.at(&s) == nullptr)
        return _emptyNCG;

    return _stops.at(&s)->pl().getSI()->getGroup()->getNodeCands(&s);
}

LINE shape_builder::get_shape_line(const node_candidate_route& ncr,
                          const routing_attributes& rAttrs)
{
    try
    {
        const edge_list_hops& res = route(ncr, rAttrs);

        LINE l;
        for (const auto& hop : res)
        {
            const trgraph::node* last = hop.start;
            if (hop.edges.empty())
            {
                l.push_back(*hop.start->pl().getGeom());
                l.push_back(*hop.end->pl().getGeom());
            }
            for (auto i = hop.edges.rbegin(); i != hop.edges.rend(); i++)
            {
                const auto* e = *i;
                if ((e->getFrom() == last) ^ e->pl().isRev())
                {
                    l.insert(l.end(), e->pl().getGeom()->begin(),
                             e->pl().getGeom()->end());
                }
                else
                {
                    l.insert(l.end(), e->pl().getGeom()->rbegin(),
                             e->pl().getGeom()->rend());
                }
                last = e->getOtherNd(last);
            }
        }

        return l;
    }
    catch (const std::runtime_error& e)
    {
        LOG(ERROR) << e.what();
        return LINE();
    }
}

LINE shape_builder::get_shape_line(Trip& trip)
{
    return get_shape_line(get_node_candidate_route(trip), getRAttrs(trip));
}

edge_list_hops shape_builder::route(const node_candidate_route& ncr,
                                   const routing_attributes& rAttrs) const
{
    graph g;

    if (_cfg.solveMethod == "global")
    {
        const edge_list_hops& ret = _crouter.route(ncr, rAttrs, _motCfg.routingOpts, _restr, &g);

        // write combination graph
        if (!_cfg.shapeTripId.empty() && _cfg.writeCombGraph)
        {
            LOG(INFO) << "Outputting combgraph.json...";
            std::ofstream pstr(_cfg.dbgOutputPath + "/combgraph.json");
            GeoGraphJsonOutput o;
            o.printLatLng(g, pstr);
        }

        return ret;
    }
    else if (_cfg.solveMethod == "greedy")
    {
        return _crouter.routeGreedy(ncr, rAttrs, _motCfg.routingOpts, _restr);
    }
    else if (_cfg.solveMethod == "greedy2")
    {
        return _crouter.routeGreedy2(ncr, rAttrs, _motCfg.routingOpts, _restr);
    }
    else
    {
        LOG(ERROR) << "Unknown solution method " << _cfg.solveMethod;
        exit(1);
    }
}

pfaedle::router::shape shape_builder::get_shape(Trip& trip) const
{
    LOG(TRACE) << "Map-matching get_shape for trip #" << trip.getId() << " of mot "
               << trip.getRoute()->getType() << "(sn=" << trip.getShortname()
               << ", rsn=" << trip.getRoute()->getShortName()
               << ", rln=" << trip.getRoute()->getLongName() << ")";
    pfaedle::router::shape ret;
    ret.hops = route(get_node_candidate_route(trip), getRAttrs(trip));
    ret.avgHopDist = get_average_hop_distance(trip);

    LOG(TRACE) << "Finished map-matching for #" << trip.getId();

    return ret;
}

pfaedle::router::shape shape_builder::get_shape(Trip& trip)
{
    LOG(TRACE) << "Map-matching get_shape for trip #" << trip.getId() << " of mot "
               << trip.getRoute()->getType() << "(sn=" << trip.getShortname()
               << ", rsn=" << trip.getRoute()->getShortName()
               << ", rln=" << trip.getRoute()->getLongName() << ")";

    pfaedle::router::shape ret;
    ret.hops = route(get_node_candidate_route(trip), getRAttrs(trip));
    ret.avgHopDist = get_average_hop_distance(trip);

    LOG(TRACE) << "Finished map-matching for #" << trip.getId();

    return ret;
}

void shape_builder::get_shape(pfaedle::netgraph::graph& ng)
{
    transit_graph_edges gtfsGraph;

    LOG(DEBUG) << "Clustering trips...";
    clusters clusters = cluster_trips(_feed, _mots);
    LOG(DEBUG) << "Clustered trips into " << clusters.size() << " clusters.";

    std::map<std::string, size_t> shpUsage;
    for (auto t : _feed.getTrips())
    {
        if (!t.getShape().empty()) shpUsage[t.getShape()]++;
    }

    // to avoid unfair load balance on threads
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(clusters.begin(), clusters.end(), g);

    size_t iters = EDijkstra::ITERS;
    size_t totiters = EDijkstra::ITERS;
    size_t oiters = EDijkstra::ITERS;
    size_t j = 0;

    auto t1 = TIME();
    auto t2 = TIME();
    double tot_avg_dist = 0;
    size_t tot_num_trips = 0;

#pragma omp parallel for num_threads(_numThreads)
    for (size_t i = 0; i < clusters.size(); i++)
    {
        j++;

        if (j % 10 == 0)
        {
#pragma omp critical
            {
                LOG(INFO) << "@ " << j << " / " << clusters.size() << " ("
                          << (static_cast<int>((j * 1.0) / clusters.size() * 100))
                          << "%, " << (EDijkstra::ITERS - oiters) << " iters, "
                          << "matching " << (10.0 / (TOOK(t1, TIME()) / 1000))
                          << " trips/sec)";

                oiters = EDijkstra::ITERS;
                t1 = TIME();
            }
        }

        // explicitly call const version of shape here for thread safety
        const pfaedle::router::shape& cshp =
                const_cast<const shape_builder&>(*this).get_shape(*clusters[i][0]);
        tot_avg_dist += cshp.avgHopDist;

        if (_cfg.buildTransitGraph)
        {
#pragma omp critical
            {
                write_transit_graph(cshp, gtfsGraph, clusters[i]);
            }
        }

        std::vector<double> distances;
        const ad::cppgtfs::gtfs::Shape& shp = get_gtfs_shape(cshp, *clusters[i][0], distances);

        LOG(TRACE) << "Took " << EDijkstra::ITERS - iters << " iterations.";
        iters = EDijkstra::ITERS;

        tot_num_trips += clusters[i].size();

        for (auto t : clusters[i])
        {
            if (_cfg.evaluate)
            {
                std::lock_guard<std::mutex> guard(_shpMutex);
                _ecoll.add(*t,
                           _evalFeed.getShapes().get(t->getShape()),
                           shp,
                           distances);
            }

            if (!t->getShape().empty() && shpUsage[t->getShape()] > 0)
            {
                shpUsage[t->getShape()]--;
                if (shpUsage[t->getShape()] == 0)
                {
                    std::lock_guard<std::mutex> guard(_shpMutex);
                    _feed.getShapes().remove(t->getShape());
                }
            }
            set_shape(*t, shp, distances);
        }
    }

    LOG(INFO) << "Matched " << tot_num_trips << " trips in " << clusters.size()
              << " clusters.";
    LOG(DEBUG) << "Took " << (EDijkstra::ITERS - totiters)
               << " iterations in total.";
    LOG(DEBUG) << "Took " << TOOK(t2, TIME()) << " ms in total.";
    LOG(DEBUG) << "Total avg. tput "
               << (static_cast<double>(EDijkstra::ITERS - totiters)) / TOOK(t2, TIME())
               << " iters/sec";
    LOG(DEBUG) << "Total avg. trip tput "
               << (clusters.size() / (TOOK(t2, TIME()) / 1000)) << " trips/sec";
    LOG(DEBUG) << "Avg hop distance was "
               << (tot_avg_dist / static_cast<double>(clusters.size()))
               << " meters";

    if (_cfg.buildTransitGraph)
    {
        LOG(INFO) << "Building transit network graph...";
        build_transit_graph(gtfsGraph, ng);
    }
}

void shape_builder::set_shape(Trip& t, const ad::cppgtfs::gtfs::Shape& s, const std::vector<double>& dists)
{
    assert(dists.size() == t.getStopTimes().size());
    // set distances
    size_t i = 0;
    for (const auto& st : t.getStopTimes())
    {
        const_cast<StopTime<Stop>&>(st).setShapeDistanceTravelled(dists[i]);
        i++;
    }

    std::lock_guard<std::mutex> guard(_shpMutex);
    t.setShape(_feed.getShapes().add(s));
}

ad::cppgtfs::gtfs::Shape shape_builder::get_gtfs_shape(
        const pfaedle::router::shape& shp,
        Trip& t,
        std::vector<double>& hopDists)
{
    ad::cppgtfs::gtfs::Shape ret(get_free_shapeId(t));

    assert(shp.hops.size() == t.getStopTimes().size() - 1);

    size_t seq = 0;
    double dist = -1;
    double lastDist = -1;
    hopDists.push_back(0);
    POINT last(0, 0);
    for (const auto& hop : shp.hops)
    {
        const trgraph::node* l = hop.start;
        if (hop.edges.empty())
        {
            POINT ll = webMercToLatLng<PFAEDLE_PRECISION>(
                    hop.start->pl().getGeom()->getX(),
                    hop.start->pl().getGeom()->getY());

            if (dist > -0.5)
                dist += webMercMeterDist(last, *hop.start->pl().getGeom());
            else
                dist = 0;

            last = *hop.start->pl().getGeom();

            if (dist - lastDist > 0.01)
            {
                ret.addPoint(ShapePoint(ll.getY(), ll.getX(), dist, seq));
                seq++;
                lastDist = dist;
            }

            dist += webMercMeterDist(last, *hop.end->pl().getGeom());
            last = *hop.end->pl().getGeom();

            if (dist - lastDist > 0.01)
            {
                ll = webMercToLatLng<PFAEDLE_PRECISION>(
                        hop.end->pl().getGeom()->getX(),
                        hop.end->pl().getGeom()->getY());
                ret.addPoint(ShapePoint(ll.getY(), ll.getX(), dist, seq));
                seq++;
                lastDist = dist;
            }
        }

        for (auto i = hop.edges.rbegin(); i != hop.edges.rend(); i++)
        {
            const auto* e = *i;

            if ((e->getFrom() == l) ^ e->pl().isRev())
            {
                for (size_t i = 0; i < e->pl().getGeom()->size(); i++)
                {
                    const POINT& cur = (*e->pl().getGeom())[i];
                    if (dist > -0.5)
                        dist += webMercMeterDist(last, cur);
                    else
                        dist = 0;
                    last = cur;
                    if (dist - lastDist > 0.01)
                    {
                        POINT ll =
                                webMercToLatLng<PFAEDLE_PRECISION>(cur.getX(), cur.getY());
                        ret.addPoint(ShapePoint(ll.getY(), ll.getX(), dist, seq));
                        seq++;
                        lastDist = dist;
                    }
                }
            }
            else
            {
                for (int64_t i = e->pl().getGeom()->size() - 1; i >= 0; i--)
                {
                    const POINT& cur = (*e->pl().getGeom())[i];
                    if (dist > -0.5)
                        dist += webMercMeterDist(last, cur);
                    else
                        dist = 0;
                    last = cur;
                    if (dist - lastDist > 0.01)
                    {
                        POINT ll =
                                webMercToLatLng<PFAEDLE_PRECISION>(cur.getX(), cur.getY());
                        ret.addPoint(ShapePoint(ll.getY(), ll.getX(), dist, seq));
                        seq++;
                        lastDist = dist;
                    }
                }
            }
            l = e->getOtherNd(l);
        }

        hopDists.push_back(lastDist);
    }

    return ret;
}

std::string shape_builder::get_free_shapeId(Trip& t)
{
    std::string ret;
    std::lock_guard<std::mutex> guard(_shpMutex);
    while (ret.empty() || _feed.getShapes().get(ret))
    {
        _curShpCnt++;
        ret = "shp_";
        ret += std::to_string(t.getRoute()->getType());
        ret += "_" + std::to_string(_curShpCnt);
    }

    return ret;
}

const routing_attributes& shape_builder::getRAttrs(const Trip& trip)
{
    auto i = _rAttrs.find(&trip);

    if (i == _rAttrs.end())
    {
        routing_attributes ret;

        const auto& lnormzer = _motCfg.osmBuildOpts.lineNormzer;

        ret.short_name = lnormzer.norm(trip.getRoute()->getShortName());

        if (ret.short_name.empty())
            ret.short_name = lnormzer.norm(trip.getShortname());

        if (ret.short_name.empty())
            ret.short_name = lnormzer.norm(trip.getRoute()->getLongName());

        ret.from = _motCfg.osmBuildOpts.statNormzer.norm(
                trip.getStopTimes().begin()->getStop()->getName());
        ret.to = _motCfg.osmBuildOpts.statNormzer.norm(
                (--trip.getStopTimes().end())->getStop()->getName());

        return _rAttrs
                .insert(std::pair<const Trip*, routing_attributes>(&trip, ret))
                .first->second;
    }
    else
    {
        return i->second;
    }
}

const routing_attributes& shape_builder::getRAttrs(const Trip& trip) const
{
    return _rAttrs.find(&trip)->second;
}

void shape_builder::get_gtfs_box(const Feed& feed, const MOTs& mots,
                              const std::string& tid, bool dropShapes,
                              osm::BBoxIdx& box)
{
    for (const auto& t : feed.getTrips())
    {
        if (!tid.empty() && t.getId() != tid)
            continue;

        if (tid.empty() && !t.getShape().empty() && !dropShapes)
            continue;

        if (t.getStopTimes().size() < 2)
            continue;

        if (mots.count(t.getRoute()->getType()))
        {
            DBox cur;
            for (const auto& st : t.getStopTimes())
            {
                cur = extendBox(DPoint(st.getStop()->getLng(), st.getStop()->getLat()),
                                cur);
            }
            box.add(cur);
        }
    }
}

node_candidate_route shape_builder::get_node_candidate_route(Trip& trip) const
{
    node_candidate_route ncr(trip.getStopTimes().size());

    size_t i = 0;

    for (const auto& st : trip.getStopTimes())
    {
        ncr[i] = getNodeCands(*st.getStop());
        if (ncr[i].empty())
        {
            throw std::runtime_error("No node candidate found for station '" +
                                     st.getStop()->getName() + "' on trip '" +
                                     trip.getId() + "'");
        }
        i++;
    }
    return ncr;
}

double shape_builder::get_average_hop_distance(Trip& trip) const
{
    size_t i = 0;
    double sum = 0;

    const Stop* prev = nullptr;

    for (const auto& st : trip.getStopTimes())
    {
        if (!prev)
        {
            prev = st.getStop();
            continue;
        }
        auto a = util::geo::latLngToWebMerc(prev->getLat(), prev->getLng());
        auto b = util::geo::latLngToWebMerc(st.getStop()->getLat(), st.getStop()->getLng());
        sum += util::geo::webMercMeterDist(a, b);

        prev = st.getStop();
        i++;
    }
    return sum / static_cast<double>(i);
}

clusters shape_builder::cluster_trips(Feed& f, const MOTs& mots)
{
    // building an index [start station, end station] -> [cluster]

    std::map<stop_pair, std::vector<size_t>> cluster_idx;

    clusters ret;
    for (auto& trip : f.getTrips())
    {
        if (!trip.getShape().empty() && !_cfg.dropShapes)
            continue;
        if (trip.getStopTimes().size() < 2)
            continue;
        if (!mots.count(trip.getRoute()->getType()) || !_motCfg.mots.count(trip.getRoute()->getType()))
            continue;

        bool found = false;
        stop_pair pair(trip.getStopTimes().begin()->getStop(),
                      trip.getStopTimes().rbegin()->getStop());
        const auto& c = cluster_idx[pair];

        for (auto i : c)
        {
            if (routingEqual(*ret[i][0], trip))
            {
                ret[i].push_back(&trip);
                found = true;
                break;
            }
        }
        if (!found)
        {
            ret.push_back(cluster{&trip});
            // explicit call to write render attrs to cache
            getRAttrs(trip);
            cluster_idx[pair].push_back(ret.size() - 1);
        }
    }

    return ret;
}

bool shape_builder::routingEqual(const Stop& a, const Stop& b)
{
    if (&a == &b) return true;// trivial

    auto namea = _motCfg.osmBuildOpts.statNormzer.norm(a.getName());
    auto nameb = _motCfg.osmBuildOpts.statNormzer.norm(b.getName());
    if (namea != nameb) return false;

    auto tracka = _motCfg.osmBuildOpts.trackNormzer.norm(a.getPlatformCode());
    auto trackb = _motCfg.osmBuildOpts.trackNormzer.norm(b.getPlatformCode());
    if (tracka != trackb) return false;

    POINT ap = util::geo::latLngToWebMerc<double>(a.getLat(), a.getLng());
    POINT bp = util::geo::latLngToWebMerc<double>(b.getLat(), b.getLng());

    double d = util::geo::webMercMeterDist(ap, bp);

    if (d > 1) return false;

    return true;
}

bool shape_builder::routingEqual(Trip& a, Trip& b)
{
    if (a.getStopTimes().size() != b.getStopTimes().size())
        return false;
    if (getRAttrs(a) != getRAttrs(b))
        return false;

    auto stb = b.getStopTimes().begin();
    for (const auto& sta : a.getStopTimes())
    {
        if (!routingEqual(*sta.getStop(), *stb->getStop()))
        {
            return false;
        }
        stb++;
    }

    return true;
}

const pfaedle::trgraph::graph& shape_builder::get_graph() const { return _g; }

void shape_builder::write_transit_graph(const pfaedle::router::shape& shp, transit_graph_edges& edgs,
                                     const cluster& cluster) const
{
    for (const auto& hop : shp.hops)
    {
        for (const auto* e : hop.edges)
        {
            if (e->pl().isRev()) e = _g.getEdg(e->getTo(), e->getFrom());
            edgs[e].insert(cluster.begin(), cluster.end());
        }
    }
}

void shape_builder::build_transit_graph(transit_graph_edges& edgs,
                                pfaedle::netgraph::graph& ng) const
{
    std::unordered_map<trgraph::node*, pfaedle::netgraph::node*> nodes;

    for (const auto& ep : edgs)
    {
        auto e = ep.first;
        pfaedle::netgraph::node* from = nullptr;
        pfaedle::netgraph::node* to = nullptr;

        if (nodes.count(e->getFrom()))
            from = nodes[e->getFrom()];
        if (nodes.count(e->getTo()))
            to = nodes[e->getTo()];
        if (!from)
        {
            from = ng.addNd(*e->getFrom()->pl().getGeom());
            nodes[e->getFrom()] = from;
        }
        if (!to)
        {
            to = ng.addNd(*e->getTo()->pl().getGeom());
            nodes[e->getTo()] = to;
        }

        ng.addEdg(from, to, pfaedle::netgraph::edge_payload(*e->pl().getGeom(), ep.second));
    }
}
}

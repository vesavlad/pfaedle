// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_get_num_procs() 1
#endif

#include <gtfs/feed.h>

#include <pfaedle/definitions.h>
#include <pfaedle/eval/collector.h>
#include <pfaedle/osm/osm_builder.h>
#include <pfaedle/router/shape_builder.h>
#include <pfaedle/trgraph/station_group.h>
#include <util/geo/Geo.h>
#include <util/geo/output/GeoGraphJsonOutput.h>
#include <util/geo/output/GeoJsonOutput.h>
#include <util/graph/EDijkstra.h>

#include <logging/logger.h>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <fstream>
#include <utility>
#include <stdexcept>
#include <exception>
#include <unordered_map>

using util::geo::DBox;
using util::geo::DPoint;
using util::geo::extendBox;

using util::geo::latLngToWebMerc;
using util::geo::webMercMeterDist;
using util::geo::webMercToLatLng;
using util::geo::output::GeoGraphJsonOutput;
using util::graph::Dijkstra;
using util::graph::EDijkstra;

namespace pfaedle::router
{
shape_builder::shape_builder(pfaedle::gtfs::feed& feed,
                             pfaedle::gtfs::feed& evalFeed,
                             route_type_set mots,
                           const config::mot_config& motCfg,
                           eval::collector& collector,
                           trgraph::graph& graph,
                           feed_stops& stops,
                             trgraph::restrictor& restr,
                           const config::config& cfg) :
    _feed(feed),
    _evalFeed(evalFeed),
    _mots(std::move(mots)),
    _motCfg(motCfg),
    _ecoll(collector),
    _cfg(cfg),
    _g(graph),
    _crouter(std::thread::hardware_concurrency(), cfg.useCaching),
    _stops(stops),
    _curShpCnt(0),
    _numThreads{_crouter.getCacheNumber()},
    _restr(restr)
{
}

const node_candidate_group& shape_builder::get_node_candidates(const pfaedle::gtfs::stop& s) const
{
    if (_stops.find(&s) == _stops.end() || _stops.at(&s) == nullptr)
        return _emptyNCG;

    return _stops.at(&s)->pl().get_si()->get_group()->get_node_candidates(&s);
}

LINE shape_builder::get_shape_line(const node_candidate_route& ncr, const routing_attributes& rAttrs)
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
                l.push_back(*hop.start->pl().get_geom());
                l.push_back(*hop.end->pl().get_geom());
            }
            for (auto i = hop.edges.rbegin(); i != hop.edges.rend(); i++)
            {
                const auto* e = *i;
                if ((e->getFrom() == last) ^ e->pl().is_reversed())
                {
                    l.insert(l.end(), e->pl().get_geom()->begin(),
                             e->pl().get_geom()->end());
                }
                else
                {
                    l.insert(l.end(), e->pl().get_geom()->rbegin(),
                             e->pl().get_geom()->rend());
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

LINE shape_builder::get_shape_line(pfaedle::gtfs::trip& trip)
{
    return get_shape_line(get_node_candidate_route(trip), getRAttrs(trip));
}

edge_list_hops shape_builder::route(const node_candidate_route& ncr,
                                   const routing_attributes& rAttrs) const
{
    graph g;

    if (_cfg.solveMethod == "global")
    {
        const edge_list_hops& ret = _crouter.route(ncr, rAttrs, _motCfg.routingOpts, _restr, g);

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

pfaedle::router::shape shape_builder::get_shape(pfaedle::gtfs::trip& trip) const
{
    if(trip.route().has_value())
    {
        const gtfs::route& r = trip.route().value();
        LOG(TRACE) << "Map-matching get_shape for trip #" << trip.trip_id << " of mot "
                   << static_cast<size_t>(r.route_type) << "(sn=" << trip.trip_short_name
                   << ", rsn=" << r.route_short_name
                   << ", rln=" << r.route_long_name << ")";
    }
    pfaedle::router::shape ret;
    ret.hops = route(get_node_candidate_route(trip), getRAttrs(trip));
    ret.avgHopDist = get_average_hop_distance(trip);

    LOG(TRACE) << "Finished map-matching for #" << trip.trip_id;

    return ret;
}

pfaedle::router::shape shape_builder::get_shape(pfaedle::gtfs::trip& trip)
{
    if(trip.route().has_value())
    {
        const gtfs::route& r = trip.route().value();
        LOG(TRACE) << "Map-matching get_shape for trip #" << trip.trip_id << " of mot "
                   <<  static_cast<size_t>(r.route_type)  << "(sn=" << trip.trip_short_name
                   << ", rsn=" << r.route_short_name
                   << ", rln=" << r.route_long_name << ")";
    }
    pfaedle::router::shape ret;
    ret.hops = route(get_node_candidate_route(trip), getRAttrs(trip));
    ret.avgHopDist = get_average_hop_distance(trip);

    LOG(TRACE) << "Finished map-matching for #" << trip.trip_id;

    return ret;
}

void shape_builder::get_shape(pfaedle::netgraph::graph& ng)
{
    transit_graph_edges gtfsGraph;

    LOG(DEBUG) << "Clustering trips...";
    clusters clusters = cluster_trips(_feed, _mots);
    LOG(DEBUG) << "Clustered trips into " << clusters.size() << " clusters.";

    std::map<std::string, size_t> shpUsage;
    for (auto& trip_pair : _feed.trips)
    {
        auto& t = trip_pair.second;
        if(!t.shape().has_value())
            continue;
        gtfs::shape& shape = t.shape()->get();
        if (!shape.points.empty())
            shpUsage[shape.shape_id]++;
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

//#pragma omp parallel for num_threads(_numThreads)
    for (size_t i = 0; i < clusters.size(); i++)
    {
        j++;

        if (j % 10 == 0)
        {
//#pragma omp critical
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
        const pfaedle::router::shape cshp =
                const_cast<const shape_builder&>(*this).get_shape(*clusters[i][0]);
        tot_avg_dist += cshp.avgHopDist;

        if (_cfg.buildTransitGraph)
        {
//#pragma omp critical
            {
                write_transit_graph(cshp, gtfsGraph, clusters[i]);
            }
        }

        std::vector<double> distances;
        std::vector<double> times;
        std::vector<double> costs;
        const pfaedle::gtfs::shape& shp = get_gtfs_shape(cshp, *clusters[i][0], distances, times, costs);

        LOG(TRACE) << "Took " << EDijkstra::ITERS - iters << " iterations.";
        iters = EDijkstra::ITERS;

        tot_num_trips += clusters[i].size();

        for (auto t : clusters[i])
        {
            if (_cfg.evaluate)
            {
                std::lock_guard<std::mutex> guard(_shpMutex);
                if(_evalFeed.shapes.count(t->shape_id))
                {
                    _ecoll.add(*t,
                               &_evalFeed.shapes.at(t->shape_id),
                               shp,
                               distances);
                }
                else
                {
                    _ecoll.add(*t,
                               nullptr,
                               shp,
                               distances);
                }
            }

            if(t->shape().has_value())
            {
                gtfs::shape& shape = t->shape().value();
                if (!shape.empty() && shpUsage[shape.shape_id] > 0)
                {
                    shpUsage[t->shape_id]--;
                    if (shpUsage[t->shape_id] == 0)
                    {
                        std::lock_guard<std::mutex> guard(_shpMutex);
                        _feed.shapes.erase(t->shape_id);
                    }
                }
            }
            set_shape(*t, shp, distances, costs);

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

void shape_builder::set_shape(pfaedle::gtfs::trip& t, const pfaedle::gtfs::shape& s, const std::vector<double>& dists, const std::vector<double>& costs)
{
    const auto& stop_times = t.stop_times();
    assert(dists.size() == stop_times.size() && costs.size() == stop_times.size());

    double total_cost = 0.f;
    for (auto& n : costs) total_cost += n;
    static constexpr size_t dwell_time_seconds = 10;

    const gtfs::stop_time& st_begin = *stop_times.begin();
    const gtfs::stop_time& st_end = *(stop_times.end() - 1);
    const size_t time_span = st_end.departure_time.get_total_seconds() - st_begin.departure_time.get_total_seconds();

    // set distances
    size_t i = 0;
    size_t previous_time = st_begin.departure_time.get_total_seconds();
    for (gtfs::stop_time& st : stop_times)
    {
        if (&st != &st_begin && &st != &st_end &&
                (!st.arrival_time.is_provided() || !st.departure_time.is_provided() || _cfg.interpolate_times))
        {
            previous_time = (time_span * costs[i] / total_cost) + previous_time;
            gtfs::time time(previous_time);
#if 0
            LOG(INFO) << "Reaching stop " << st.stop()->get().stop_name
                      << " \n\t - has a calculated cost of " << costs[i]
                      << " \n\t - has a calculated time of " << time.get_raw_time()
                      << " \n\t - and a provided time of " << st.departure_time.get_raw_time();
            LOG(INFO) << "Reaching stop " << st.stop()->get().stop_name << " has a distance of " << dists[i];
#endif
            st.arrival_time = time;
            st.departure_time = time.add_seconds(dwell_time_seconds);
        }

        st.shape_dist_traveled = dists[i];
        i++;
    }

    std::lock_guard guard(_shpMutex);
    _feed.shapes.emplace(s.shape_id, s);
    t.shape_id = s.shape_id;
}

pfaedle::gtfs::shape shape_builder::get_gtfs_shape(
        const pfaedle::router::shape& shp,
        pfaedle::gtfs::trip& t,
        std::vector<double>& hopDists,
        std::vector<double>& hopTimes,
        std::vector<double>& costs)
{
    pfaedle::gtfs::shape ret;
    ret.shape_id = get_free_shapeId(t);

    assert(shp.hops.size() == t.stop_times().size() - 1);

    size_t seq = 0;

    double time = 0;
    double lastTime = 0;
    double last_speed = 50.f;

    double dist = -1;
    double lastDist = -1;

    hopDists.push_back(0);
    costs.push_back(0);
    POINT last(0, 0);
    for (const auto& hop : shp.hops)
    {
        const trgraph::node* node = hop.start;
        if (hop.edges.empty())
        {
            POINT ll = webMercToLatLng(
                    hop.start->pl().get_geom()->getX(),
                    hop.start->pl().get_geom()->getY());

            if (dist > -0.5)
            {
                const double distance_between_points = webMercMeterDist(last, *hop.start->pl().get_geom());
                time += 3.6f * distance_between_points / last_speed;
                dist += distance_between_points;
            }
            else
            {
                dist = 0;
                time = 0;
            }

            last = *hop.start->pl().get_geom();

            if (dist - lastDist > 0.01)
            {
                gtfs::shape_point point{ret.shape_id, ll.getY(), ll.getX(), seq, dist};
                ret.points.push_back(point);
                seq++;
                lastDist = dist;
                lastTime = time;
            }

            const double distance_between_points = webMercMeterDist(last, *hop.end->pl().get_geom());
            time += 3.6f * distance_between_points / last_speed;
            dist += distance_between_points;
            last = *hop.end->pl().get_geom();

            if (dist - lastDist > 0.01)
            {
                ll = webMercToLatLng(
                        hop.end->pl().get_geom()->getX(),
                        hop.end->pl().get_geom()->getY());
                gtfs::shape_point point{ret.shape_id, ll.getY(), ll.getX(), seq, dist};
                ret.points.push_back(point);
                seq++;
                lastDist = dist;
                lastTime = time;
            }
        }

        for (auto it = hop.edges.rbegin(); it != hop.edges.rend(); it++)
        {
            const auto* edge = *it;

            // time = 3.6f * distance(m) / speed (km/h)
            last_speed = edge->pl().get_max_speed() - 10.0f;

            if ((edge->getFrom() == node) ^ edge->pl().is_reversed())
            {
                for (size_t i = 0; i < edge->pl().get_geom()->size(); i++)
                {
                    const POINT& cur = (*edge->pl().get_geom())[i];
                    if (dist > -0.5)
                    {
                        const double distance_between_points = webMercMeterDist(last, cur);
                        time += 3.6f * distance_between_points / last_speed;
                        dist += distance_between_points;
                    }else
                    {
                        dist = 0;
                        time = 0;
                    }

                    last = cur;
                    if (dist - lastDist > 0.01)
                    {
                        POINT ll = webMercToLatLng(cur.getX(), cur.getY());
                        gtfs::shape_point point{ret.shape_id, ll.getY(), ll.getX(), seq, dist};
                        ret.points.push_back(point);

                        seq++;
                        lastDist = dist;
                        lastTime = time;
                    }
                }
            }
            else
            {
                for (int64_t i = edge->pl().get_geom()->size() - 1; i >= 0; i--)
                {
                    const POINT& cur = (*edge->pl().get_geom())[i];
                    if (dist > -0.5)
                    {
                        const double distance_between_points = webMercMeterDist(last, cur);
                        time += 3.6f * distance_between_points / last_speed;
                        dist += distance_between_points;
                    }else
                    {
                        dist = 0;
                        time = 0;
                    }
                    last = cur;
                    if (dist - lastDist > 0.01)
                    {
                        POINT ll = webMercToLatLng(cur.getX(), cur.getY());
                        gtfs::shape_point point{ret.shape_id, ll.getY(), ll.getX(), seq, dist};
                        ret.points.push_back(point);
                        seq++;
                        lastDist = dist;
                        lastTime = time;
                    }
                }
            }
            node = edge->getOtherNd(node);
        }

        hopDists.push_back(lastDist);
        hopTimes.push_back(lastTime);
        costs.push_back(hop.cost);
    }

    return ret;
}

std::string shape_builder::get_free_shapeId(pfaedle::gtfs::trip& t)
{
    std::string ret;
    std::lock_guard<std::mutex> guard(_shpMutex);
    while (ret.empty() || _feed.shapes.count(ret))
    {
        _curShpCnt++;
        ret = "shp_";
        if(t.route().has_value())
        {
            gtfs::route& r = t.route()->get();
            ret += r.route_id + "_" + std::to_string(static_cast<size_t>(r.route_type));
        }
        else
        {
            ret += t.trip_id;
        }
        ret += "_" + std::to_string(_curShpCnt);
    }

    return ret;
}

const routing_attributes& shape_builder::getRAttrs(const pfaedle::gtfs::trip& trip)
{
    auto i = _rAttrs.find(&trip);

    if (i == _rAttrs.end())
    {
        routing_attributes ret;
        if(!trip.route().has_value())
            throw std::runtime_error("trip has not route associated, didn't expect that!");

        const gtfs::route& route = trip.route()->get();

        const auto& lnormzer = _motCfg.osmBuildOpts.lineNormzer;

        ret.short_name = lnormzer.norm(route.route_short_name);

        if (ret.short_name.empty())
            ret.short_name = lnormzer.norm(route.route_short_name);

        if (ret.short_name.empty())
            ret.short_name = lnormzer.norm(route.route_long_name);


        ret.from = _motCfg.osmBuildOpts.statNormzer.norm(trip.stop_times().begin()->get().stop()->get().stop_name);
        ret.to = _motCfg.osmBuildOpts.statNormzer.norm((--trip.stop_times().end())->get().stop()->get().stop_name);

        return _rAttrs
                .insert(std::pair<const gtfs::trip*, routing_attributes>(&trip, ret))
                .first->second;
    }
    else
    {
        return i->second;
    }
}

const routing_attributes& shape_builder::getRAttrs(const pfaedle::gtfs::trip& trip) const
{
    return _rAttrs.find(&trip)->second;
}

void shape_builder::get_gtfs_box(const pfaedle::gtfs::feed& feed,
                                 const route_type_set& mots,
                                 const std::string& tid,
                                 bool dropShapes,
                                 osm::bounding_box& box)
{
    for (const auto& trip_pair : feed.trips)
    {
        auto& t = trip_pair.second;
        if (!tid.empty() && t.trip_id != tid)
            continue;

        if (tid.empty() && t.shape().has_value() && !t.shape().value().get().points.empty() && !dropShapes)
            continue;

        if (t.stop_times().size() < 2)
            continue;

        const auto& route = t.route();
        if (route.has_value() && mots.count(route.value().get().route_type))
        {
            DBox cur;
            for (const auto& st : t.stop_times())
            {
                pfaedle::gtfs::stop_time& stop_time = st;
                if(stop_time.stop().has_value())
                {
                    pfaedle::gtfs::stop& s = stop_time.stop().value();
                    cur = extendBox(DPoint(s.stop_lon, s.stop_lat), cur);
                }
            }
            box.add(cur);
        }
    }
}

node_candidate_route shape_builder::get_node_candidate_route(pfaedle::gtfs::trip& trip) const
{
    const auto& trip_stop_times = trip.stop_times();
    node_candidate_route ncr(trip_stop_times.size());

    size_t i = 0;

    for (const gtfs::stop_time& st : trip_stop_times)
    {

        ncr[i] = get_node_candidates(st.stop().value());
        if (ncr[i].empty())
        {
            throw std::runtime_error("No node candidate found for station '" +
                                     st.stop()->get().stop_name + "' on trip '" +
                                     trip.trip_id + "'");
        }
        i++;
    }
    return ncr;
}

double shape_builder::get_average_hop_distance(pfaedle::gtfs::trip& trip) const
{
    size_t i = 0;
    double sum = 0;

    const pfaedle::gtfs::stop* prev = nullptr;

    for (const gtfs::stop_time& st : trip.stop_times())
    {
        if (!prev)
        {
            prev = &st.stop()->get();
            continue;
        }
        const gtfs::stop& current = st.stop()->get();

        auto a = util::geo::latLngToWebMerc(prev->stop_lat, prev->stop_lon);
        auto b = util::geo::latLngToWebMerc(current.stop_lat, current.stop_lon);
        sum += util::geo::webMercMeterDist(a, b);

        prev = &current;
        i++;
    }
    return sum / static_cast<double>(i);
}

clusters shape_builder::cluster_trips(pfaedle::gtfs::feed& f, const route_type_set & mots)
{
    // building an index [start station, end station] -> [cluster]

    std::map<stop_pair, std::vector<size_t>> cluster_idx;

    clusters ret;
    for (auto& trip_pair : f.trips)
    {
        auto& trip = trip_pair.second;
        if(trip.shape().has_value() && !trip.shape()->get().empty() && !_cfg.dropShapes)
            continue;

        if (trip.stop_times().size() < 2)
            continue;
        if(!trip.route().has_value())
            continue;

        gtfs::route& r = trip.route()->get();
        if (!mots.count(r.route_type) || !_motCfg.route_types.count(r.route_type))
            continue;

        bool found = false;
        stop_pair pair(&trip.stop_times().begin()->get().stop()->get(),
                       &trip.stop_times().rbegin()->get().stop()->get());
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

bool shape_builder::routingEqual(const pfaedle::gtfs::stop& a, const pfaedle::gtfs::stop& b)
{
    if (&a == &b) return true;// trivial

    auto namea = _motCfg.osmBuildOpts.statNormzer.norm(a.stop_name);
    auto nameb = _motCfg.osmBuildOpts.statNormzer.norm(b.stop_name);
    if (namea != nameb) return false;

    auto tracka = _motCfg.osmBuildOpts.trackNormzer.norm(a.platform_code);
    auto trackb = _motCfg.osmBuildOpts.trackNormzer.norm(b.platform_code);
    if (tracka != trackb) return false;

    POINT ap = util::geo::latLngToWebMerc(a.stop_lat, a.stop_lon);
    POINT bp = util::geo::latLngToWebMerc(b.stop_lat, b.stop_lon);

    double d = util::geo::webMercMeterDist(ap, bp);

    if (d > 1) return false;

    return true;
}

bool shape_builder::routingEqual(pfaedle::gtfs::trip& a, pfaedle::gtfs::trip& b)
{
    const auto& ast_list = a.stop_times();
    const auto& bst_list = b.stop_times();

    if (ast_list.size() != bst_list.size())
        return false;
    if (getRAttrs(a) != getRAttrs(b))
        return false;

    auto stb = bst_list.begin();
    for (const auto& sta : ast_list)
    {
        if (!routingEqual(*sta.get().stop(), *stb->get().stop()))
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
            if (e->pl().is_reversed()) e = _g.getEdg(e->getTo(), e->getFrom());
            edgs[e].insert(cluster.begin(), cluster.end());
        }
    }
}

void shape_builder::build_transit_graph(transit_graph_edges& edgs, pfaedle::netgraph::graph& ng) const
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
            from = ng.addNd(*e->getFrom()->pl().get_geom());
            nodes[e->getFrom()] = from;
        }
        if (!to)
        {
            to = ng.addNd(*e->getTo()->pl().get_geom());
            nodes[e->getTo()] = to;
        }

        ng.addEdg(from, to, pfaedle::netgraph::edge_payload(*e->pl().get_geom(), ep.second));
    }
}
}

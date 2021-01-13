// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/station_group.h"
#include "util/geo/Geo.h"
#include <set>

using ad::cppgtfs::gtfs::Stop;
using pfaedle::router::node_candidate_group;

namespace pfaedle::trgraph
{

station_group::station_group() = default;

void station_group::add_stop(const Stop* s) { _stops.insert(s); }

void station_group::add_node(trgraph::node* n) { _nodes.insert(n); }

void station_group::merge(station_group* other)
{
    if (other == this) return;

    std::set<node*> nds = other->get_nodes();
    std::set<const Stop*> stops = other->get_stops();

    for (auto on : nds)
    {
        on->pl().get_si()->set_group(this);
        add_node(on);
    }

    for (auto* os : stops)
    {
        add_stop(os);
    }
}

const node_candidate_group& station_group::get_node_candidates(const Stop* s) const
{
    return _stopNodePens.at(s);
}

const std::set<node*>& station_group::get_nodes() const { return _nodes; }

void station_group::remove_node(trgraph::node* n)
{
    auto it = _nodes.find(n);
    if (it != _nodes.end()) _nodes.erase(it);
}

std::set<node*>& station_group::get_nodes() { return _nodes; }

const std::set<const Stop*>& station_group::get_stops() const { return _stops; }

double station_group::get_penalty(const Stop* s, trgraph::node* n,
                             const trgraph::normalizer& norm,
                             double trackPen, double distPenFac,
                             double nonOsmPen) const
{
    POINT p = util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(s->getLat(), s->getLng());

    double distPen = util::geo::webMercMeterDist(p, *n->pl().get_geom());
    distPen *= distPenFac;

    std::string platform = norm.norm(s->getPlatformCode());

    if (!platform.empty() && !n->pl().get_si()->get_track().empty() &&
        n->pl().get_si()->get_track() == platform)
    {
        trackPen = 0;
    }

    if (n->pl().get_si()->is_from_osm())
        nonOsmPen = 0;

    return distPen + trackPen + nonOsmPen;
}

void station_group::write_penalties(const trgraph::normalizer& platformNorm,
                                    double trackPen, double distPenFac,
                                    double nonOsmPen)
{
    if (!_stopNodePens.empty()) return;// already written
    for (auto* s : _stops)
    {
        for (auto* n : _nodes)
        {
            _stopNodePens[s].push_back(router::node_candidate{
                    n, get_penalty(s, n, platformNorm, trackPen, distPenFac, nonOsmPen)});
        }
    }
}
}

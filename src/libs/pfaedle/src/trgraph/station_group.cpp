// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/station_group.h"
#include "util/geo/Geo.h"
#include <set>

using ad::cppgtfs::gtfs::Stop;
using pfaedle::router::node_candidate_group;
using pfaedle::trgraph::node;
using pfaedle::trgraph::station_group;

// _____________________________________________________________________________
station_group::station_group() {}

// _____________________________________________________________________________
void station_group::addStop(const Stop* s) { _stops.insert(s); }

// _____________________________________________________________________________
void station_group::addNode(trgraph::node* n) { _nodes.insert(n); }

// _____________________________________________________________________________
void station_group::merge(station_group* other)
{
    if (other == this) return;

    std::set<node*> nds = other->getNodes();
    std::set<const Stop*> stops = other->getStops();

    for (auto on : nds)
    {
        on->pl().getSI()->setGroup(this);
        addNode(on);
    }

    for (auto* os : stops)
    {
        addStop(os);
    }
}

// _____________________________________________________________________________
const node_candidate_group& station_group::getNodeCands(const Stop* s) const
{
    return _stopNodePens.at(s);
}

// _____________________________________________________________________________
const std::set<node*>& station_group::getNodes() const { return _nodes; }

// _____________________________________________________________________________
void station_group::remNode(trgraph::node* n)
{
    auto it = _nodes.find(n);
    if (it != _nodes.end()) _nodes.erase(it);
}

// _____________________________________________________________________________
std::set<node*>& station_group::getNodes() { return _nodes; }

// _____________________________________________________________________________
const std::set<const Stop*>& station_group::getStops() const { return _stops; }

// _____________________________________________________________________________
double station_group::getPen(const Stop* s, trgraph::node* n,
                         const trgraph::normalizer& platformNorm,
                         double trackPen, double distPenFac,
                         double nonOsmPen) const
{
    POINT p =
            util::geo::latLngToWebMerc<PFAEDLE_PRECISION>(s->getLat(), s->getLng());

    double distPen = util::geo::webMercMeterDist(p, *n->pl().getGeom());
    distPen *= distPenFac;

    std::string platform = platformNorm.norm(s->getPlatformCode());

    if (!platform.empty() && !n->pl().getSI()->getTrack().empty() &&
        n->pl().getSI()->getTrack() == platform)
    {
        trackPen = 0;
    }

    if (n->pl().getSI()->isFromOsm()) nonOsmPen = 0;

    return distPen + trackPen + nonOsmPen;
}

// _____________________________________________________________________________
void station_group::writePens(const trgraph::normalizer& platformNorm,
                          double trackPen, double distPenFac,
                          double nonOsmPen)
{
    if (!_stopNodePens.empty()) return;// already written
    for (auto* s : _stops)
    {
        for (auto* n : _nodes)
        {
            _stopNodePens[s].push_back(router::node_candidate{
                    n, getPen(s, n, platformNorm, trackPen, distPenFac, nonOsmPen)});
        }
    }
}

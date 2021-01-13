// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/StatGroup.h"
#include "util/geo/Geo.h"
#include <set>

using ad::cppgtfs::gtfs::Stop;
using pfaedle::router::node_candidate_group;
using pfaedle::trgraph::Node;
using pfaedle::trgraph::StatGroup;

// _____________________________________________________________________________
StatGroup::StatGroup() {}

// _____________________________________________________________________________
void StatGroup::addStop(const Stop* s) { _stops.insert(s); }

// _____________________________________________________________________________
void StatGroup::addNode(trgraph::Node* n) { _nodes.insert(n); }

// _____________________________________________________________________________
void StatGroup::merge(StatGroup* other)
{
    if (other == this) return;

    std::set<Node*> nds = other->getNodes();
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
const node_candidate_group& StatGroup::getNodeCands(const Stop* s) const
{
    return _stopNodePens.at(s);
}

// _____________________________________________________________________________
const std::set<Node*>& StatGroup::getNodes() const { return _nodes; }

// _____________________________________________________________________________
void StatGroup::remNode(trgraph::Node* n)
{
    auto it = _nodes.find(n);
    if (it != _nodes.end()) _nodes.erase(it);
}

// _____________________________________________________________________________
std::set<Node*>& StatGroup::getNodes() { return _nodes; }

// _____________________________________________________________________________
const std::set<const Stop*>& StatGroup::getStops() const { return _stops; }

// _____________________________________________________________________________
double StatGroup::getPen(const Stop* s, trgraph::Node* n,
                         const trgraph::Normalizer& platformNorm,
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
void StatGroup::writePens(const trgraph::Normalizer& platformNorm,
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

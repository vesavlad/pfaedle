// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/NodePayload.h"
#include "pfaedle/trgraph/StatGroup.h"
#include "pfaedle/trgraph/StatInfo.h"
#include "util/String.h"
#include <string>
#include <unordered_map>

using pfaedle::trgraph::Component;
using pfaedle::trgraph::NodePayload;
using pfaedle::trgraph::StatInfo;

// we use the adress of this dummy station info as a special value
// of this node, meaning "is a station block". Re-using the _si field here
// saves some memory
StatInfo NodePayload::_blockerSI = StatInfo();

std::unordered_map<const Component*, size_t> NodePayload::_comps;

// _____________________________________________________________________________
NodePayload::NodePayload() :
    _geom(0, 0),
    _si(nullptr),
    _component(nullptr)
#ifdef PFAEDLE_DBG
    ,
    _vis(0)
#endif
{
}

// _____________________________________________________________________________
NodePayload::NodePayload(const NodePayload& pl) :
    _geom(pl._geom),
    _si(nullptr),
    _component(pl._component)
#ifdef PFAEDLE_DBG
    ,
    _vis(pl._vis)
#endif
{
    if (pl._si) setSI(*(pl._si));
}

// _____________________________________________________________________________
NodePayload::NodePayload(const POINT& geom) :
    _geom(geom),
    _si(nullptr),
    _component(nullptr)
#ifdef PFAEDLE_DBG
    ,
    _vis(0)
#endif
{
}

// _____________________________________________________________________________
NodePayload::NodePayload(const POINT& geom, const StatInfo& si) :
    _geom(geom),
    _si(nullptr),
    _component(nullptr)
#ifdef PFAEDLE_DBG
    ,
    _vis(0)
#endif
{
    setSI(si);
}

// _____________________________________________________________________________
NodePayload::~NodePayload()
{
    if (getSI()) delete _si;
    if (_component)
    {
        _comps[_component]--;
        if (_comps[_component] == 0)
        {
            delete _component;
            _comps.erase(_comps.find(_component));
        }
    }
}

// _____________________________________________________________________________
void NodePayload::setVisited() const
{
#ifdef PFAEDLE_DBG
    _vis = true;
#endif
}

// _____________________________________________________________________________
void NodePayload::setNoStat() { _si = nullptr; }

// _____________________________________________________________________________
const Component* NodePayload::getComp() const { return _component; }

// _____________________________________________________________________________
void NodePayload::setComp(const Component* c)
{
    if (_component == c) return;
    _component = c;

    // NOT thread safe!
    if (!_comps.count(c))
        _comps[c] = 1;
    else
        _comps[c]++;
}

// _____________________________________________________________________________
const POINT* NodePayload::getGeom() const { return &_geom; }

// _____________________________________________________________________________
void NodePayload::setGeom(const POINT& geom) { _geom = geom; }

// _____________________________________________________________________________
util::json::Dict NodePayload::getAttrs() const
{
    util::json::Dict obj;
    obj["component"] = std::to_string(reinterpret_cast<size_t>(_component));
#ifdef PFAEDLE_DBG
    obj["dijkstra_vis"] = _vis ? "yes" : "no";
#endif
    if (getSI())
    {
        obj["station_info_ptr"] = util::toString(_si);
        obj["station_name"] = _si->getName();
        obj["station_alt_names"] = util::implode(_si->getAltNames(), ",");
        obj["from_osm"] = _si->isFromOsm() ? "yes" : "no";
        obj["station_platform"] = _si->getTrack();
        obj["station_group"] =
                std::to_string(reinterpret_cast<size_t>(_si->getGroup()));

#ifdef PFAEDLE_STATION_IDS
        // only print this in debug mode
        obj["station_id"] = _si->getId();
#endif


        std::stringstream gtfsIds;
        if (_si->getGroup())
        {
            for (auto* s : _si->getGroup()->getStops())
            {
                gtfsIds << s->getId() << " (" << s->getName() << "),";
            }
        }

        obj["station_group_stops"] = gtfsIds.str();
    }
    return obj;
}

// _____________________________________________________________________________
void NodePayload::setSI(const StatInfo& si) { _si = new StatInfo(si); }

// _____________________________________________________________________________
const StatInfo* NodePayload::getSI() const
{
    if (isBlocker()) return nullptr;
    return _si;
}

// _____________________________________________________________________________
StatInfo* NodePayload::getSI()
{
    if (isBlocker()) return nullptr;
    return _si;
}

// _____________________________________________________________________________
void NodePayload::setBlocker() { _si = &_blockerSI; }

// _____________________________________________________________________________
bool NodePayload::isBlocker() const { return _si == &_blockerSI; }

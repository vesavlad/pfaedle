// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/node_payload.h"
#include "pfaedle/trgraph/station_group.h"
#include "pfaedle/trgraph/station_info.h"
#include "util/String.h"
#include <string>
#include <unordered_map>

using pfaedle::trgraph::component;
using pfaedle::trgraph::node_payload;
using pfaedle::trgraph::station_info;

// we use the adress of this dummy station info as a special value
// of this node, meaning "is a station block". Re-using the _si field here
// saves some memory
station_info node_payload::_blockerSI = station_info();

std::unordered_map<const component*, size_t> node_payload::_comps;

// _____________________________________________________________________________
node_payload::node_payload() :
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
node_payload::node_payload(const node_payload& pl) :
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
node_payload::node_payload(const POINT& geom) :
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
node_payload::node_payload(const POINT& geom, const station_info& si) :
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
node_payload::~node_payload()
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
void node_payload::setVisited() const
{
#ifdef PFAEDLE_DBG
    _vis = true;
#endif
}

// _____________________________________________________________________________
void node_payload::setNoStat() { _si = nullptr; }

// _____________________________________________________________________________
const component* node_payload::getComp() const { return _component; }

// _____________________________________________________________________________
void node_payload::setComp(const component* c)
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
const POINT* node_payload::getGeom() const { return &_geom; }

// _____________________________________________________________________________
void node_payload::setGeom(const POINT& geom) { _geom = geom; }

// _____________________________________________________________________________
util::json::Dict node_payload::getAttrs() const
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
void node_payload::setSI(const station_info& si) { _si = new station_info(si); }

// _____________________________________________________________________________
const station_info* node_payload::getSI() const
{
    if (isBlocker()) return nullptr;
    return _si;
}

// _____________________________________________________________________________
station_info* node_payload::getSI()
{
    if (isBlocker()) return nullptr;
    return _si;
}

// _____________________________________________________________________________
void node_payload::setBlocker() { _si = &_blockerSI; }

// _____________________________________________________________________________
bool node_payload::isBlocker() const { return _si == &_blockerSI; }

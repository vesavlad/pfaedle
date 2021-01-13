// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/node_payload.h"
#include "pfaedle/trgraph/station_group.h"
#include "pfaedle/trgraph/station_info.h"
#include "util/String.h"
#include <string>
#include <unordered_map>

namespace pfaedle::trgraph
{

// we use the adress of this dummy station info as a special value
// of this node, meaning "is a station block". Re-using the _si field here
// saves some memory
station_info node_payload::_blockerSI = station_info();

std::unordered_map<const component*, size_t> node_payload::_comps;

node_payload::node_payload() :
    _geom(0, 0),
    _si(nullptr),
    _component(nullptr)
#ifdef PFAEDLE_DBG
    ,
    _vis(false)
#endif
{
}

node_payload::node_payload(const node_payload& pl) :
    _geom(pl._geom),
    _si(nullptr),
    _component(pl._component)
#ifdef PFAEDLE_DBG
    ,_vis(pl._vis)
#endif
{
    if (pl._si) set_si(*(pl._si));
}

node_payload::node_payload(const POINT& geom) :
    _geom(geom),
    _si(nullptr),
    _component(nullptr)
#ifdef PFAEDLE_DBG
    ,_vis(false)
#endif
{
}

node_payload::node_payload(const POINT& geom, const station_info& si) :
    _geom(geom),
    _si(nullptr),
    _component(nullptr)
#ifdef PFAEDLE_DBG
    ,_vis(false)
#endif
{
    set_si(si);
}

node_payload::~node_payload()
{
    if (get_si())
        delete _si;

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

void node_payload::set_visited() const
{
#ifdef PFAEDLE_DBG
    _vis = true;
#endif
}

void node_payload::set_no_statistics()
{
    _si = nullptr;
}

const component* node_payload::get_component() const
{
    return _component;
}

void node_payload::set_component(const component* c)
{
    if (_component == c)
        return;
    _component = c;

    // NOT thread safe!
    if (!_comps.count(c))
        _comps[c] = 1;
    else
        _comps[c]++;
}

const POINT* node_payload::get_geom() const
{
    return &_geom;
}

void node_payload::set_geom(const POINT& geom)
{
    _geom = geom;
}

util::json::Dict node_payload::get_attrs() const
{
    util::json::Dict obj;
    obj["component"] = std::to_string(reinterpret_cast<size_t>(_component));
#ifdef PFAEDLE_DBG
    obj["dijkstra_vis"] = _vis ? "yes" : "no";
#endif
    if (get_si())
    {
        obj["station_info_ptr"] = util::toString(_si);
        obj["station_name"] = _si->get_name();
        obj["station_alt_names"] = util::implode(_si->get_alternative_names(), ",");
        obj["from_osm"] = _si->is_from_osm() ? "yes" : "no";
        obj["station_platform"] = _si->get_track();
        obj["station_group"] =
                std::to_string(reinterpret_cast<size_t>(_si->get_group()));

#ifdef PFAEDLE_STATION_IDS
        // only print this in debug mode
        obj["station_id"] = _si->getId();
#endif


        std::stringstream gtfs_ids;
        if (_si->get_group())
        {
            for (auto* s : _si->get_group()->get_stops())
            {
                gtfs_ids << s->getId() << " (" << s->getName() << "),";
            }
        }

        obj["station_group_stops"] = gtfs_ids.str();
    }
    return obj;
}

void node_payload::set_si(const station_info& si)
{
    _si = new station_info(si);
}

const station_info* node_payload::get_si() const
{
    if (is_blocker())
        return nullptr;
    return _si;
}

station_info* node_payload::get_si()
{
    if (is_blocker())
        return nullptr;
    return _si;
}

void node_payload::set_blocker()
{
    _si = &_blockerSI;
}

bool node_payload::is_blocker() const
{
    return _si == &_blockerSI;
}
}

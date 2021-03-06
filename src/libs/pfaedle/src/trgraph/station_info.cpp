// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/station_info.h"
#include "pfaedle/router/comp.h"
#include "pfaedle/trgraph/station_group.h"

namespace pfaedle::trgraph
{

std::unordered_map<const station_group*, size_t> station_info::_groups;

station_info::station_info() :
    _name(),
    _track(),
    _fromOsm(false),
    _group(nullptr) {}

station_info::station_info(const station_info& si) :
    _name(si._name),
    _altNames(si._altNames),
    _track(si._track),
    _fromOsm(si._fromOsm),
    _group(nullptr)
{
    set_group(si._group);
#ifdef PFAEDLE_STATION_IDS
    _id = si._id;
#endif
}

station_info::station_info(const std::string& name, const std::string& track,
                           bool fromOsm) :
    _name(name),
    _track(track), _fromOsm(fromOsm), _group(nullptr) {}

station_info::~station_info() { unRefGroup(_group); }

void station_info::unRefGroup(station_group* g)
{
    if (g)
    {
        _groups[g]--;
        if (_groups[g] == 0)
        {
            // std::cout << "Deleting " << g << std::endl;
            _groups.erase(_groups.find(g));
            delete g;
        }
    }
}

void station_info::set_group(station_group* g)
{
    if (_group == g) return;
    unRefGroup(_group);

    _group = g;

    // NOT thread safe!
    if (!_groups.count(g))
        _groups[g] = 1;
    else
        _groups[g]++;
}

station_group* station_info::get_group() const { return _group; }

const std::string& station_info::get_name() const { return _name; }

const std::string& station_info::get_track() const { return _track; }

bool station_info::is_from_osm() const { return _fromOsm; }

void station_info::set_is_from_osm(bool is) { _fromOsm = is; }

double station_info::simi(const station_info* other) const
{
    if (!other) return 0;
    if (router::statSimi(_name, other->get_name()) > 0.5) return 1;

    for (const auto& a : _altNames)
    {
        if (router::statSimi(a, other->get_name()) > 0.5) return 1;
        for (const auto& b : other->get_alternative_names())
        {
            if (router::statSimi(a, b) > 0.5) return 1;
        }
    }

    for (const auto& b : other->get_alternative_names())
    {
        if (router::statSimi(_name, b) > 0.5) return 1;
    }

    return 0;
}

const std::vector<std::string>& station_info::get_alternative_names() const
{
    return _altNames;
}

void station_info::add_alternative_name(const std::string& name)
{
    _altNames.push_back(name);
}

void station_info::set_track(const std::string& tr) { _track = tr; }
}

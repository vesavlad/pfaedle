// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/osm/osm_filter.h"
#include <iostream>
#include <sstream>
#include <string>

using pfaedle::osm::osm_filter;

// _____________________________________________________________________________
osm_filter::osm_filter(const multi_attribute_map& keep, const multi_attribute_map& drop) :
    _keep(keep), _drop(drop) {}

// _____________________________________________________________________________
osm_filter::osm_filter(const osm_read_options& o) :
    _keep(o.keepFilter),
    _drop(o.dropFilter),
    _nohup(o.noHupFilter),
    _oneway(o.oneWayFilter),
    _onewayrev(o.oneWayFilterRev),
    _twoway(o.twoWayFilter),
    _station(o.stationFilter),
    _blocker(o.stationBlockerFilter),
    _posRestr(o.restrPosRestr),
    _negRestr(o.restrNegRestr),
    _noRestr(o.noRestrFilter),
    _levels(o.levelFilters) {}

// _____________________________________________________________________________
uint64_t osm_filter::keep(const attribute_map& attrs, Type t) const
{
    return contained(attrs, _keep, t);
}

// _____________________________________________________________________________
uint64_t osm_filter::drop(const attribute_map& attrs, Type t) const
{
    return contained(attrs, _drop, t);
}

// _____________________________________________________________________________
uint64_t osm_filter::nohup(const char* key, const char* v) const
{
    const auto& dkv = _nohup.find(key);
    if (dkv != _nohup.end())
    {
        for (const auto& val : dkv->second)
        {
            if (valMatches(v, val.first)) return true;
        }
    }

    return false;
}

// _____________________________________________________________________________
uint64_t osm_filter::oneway(const attribute_map& attrs) const
{
    if (contained(attrs, _twoway, WAY)) return false;
    return contained(attrs, _oneway, WAY);
}

// _____________________________________________________________________________
uint64_t osm_filter::onewayrev(const attribute_map& attrs) const
{
    if (contained(attrs, _twoway, WAY)) return false;
    return contained(attrs, _onewayrev, WAY);
}

// _____________________________________________________________________________
uint64_t osm_filter::station(const attribute_map& attrs) const
{
    return contained(attrs, _station, NODE);
}

// _____________________________________________________________________________
uint64_t osm_filter::blocker(const attribute_map& attrs) const
{
    return contained(attrs, _blocker, NODE);
}

// _____________________________________________________________________________
uint64_t osm_filter::contained(const attribute_map& attrs, const multi_attribute_map& map,
                              Type t)
{
    for (const auto& kv : attrs)
    {
        const auto& dkv = map.find(kv.first);

        if (dkv != map.end())
        {
            for (const auto& val : dkv->second)
            {
                bool multValMatch = val.second & osm::MULT_VAL_MATCH;
                if (val.second & t) continue;
                if (valMatches(kv.second, val.first, multValMatch)) return val.second;
            }
        }
    }

    return 0;
}

// _____________________________________________________________________________
uint64_t osm_filter::contained(const attribute_map& attrs, const attribute& attr)
{
    for (const auto& kv : attrs)
    {
        if (kv.first == attr.first) return valMatches(kv.second, attr.second);
    }

    return 0;
}

// _____________________________________________________________________________
uint8_t osm_filter::level(const attribute_map& attrs) const
{
    // the best matching level is always returned
    for (int16_t i = 0; i < 8; i++)
    {
        for (const auto& kv : attrs)
        {
            const auto& lkv = (_levels + i)->find(kv.first);
            if (lkv != (_levels + i)->end())
            {
                for (const auto& val : lkv->second)
                {
                    if (valMatches(kv.second, val.first)) return i;
                }
            }
        }
    }

    return 0;
}

// _____________________________________________________________________________
bool osm_filter::valMatches(const std::string& a, const std::string& b)
{
    return valMatches(a, b, false);
}

// _____________________________________________________________________________
bool osm_filter::valMatches(const std::string& a, const std::string& b, bool m)
{
    if (b == "*") return true;

    if (m)
    {
        // search for occurances in semicolon separated list
        if (a.find(std::string(";") + b) != std::string::npos)
            return true;
        if (a.find(b + ";") != std::string::npos)
            return true;
        if (a.find(std::string("; ") + b) != std::string::npos)
            return true;
        if (a.find(b + " ;") != std::string::npos)
            return true;
    }

    return a == b;
}

// _____________________________________________________________________________
std::vector<std::string> osm_filter::getAttrKeys() const
{
    std::vector<std::string> ret;
    for (const auto& kv : _keep)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _drop)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _nohup)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _oneway)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _twoway)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _station)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _blocker)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _posRestr)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _negRestr)
    {
        ret.push_back(kv.first);
    }
    for (const auto& kv : _noRestr)
    {
        ret.push_back(kv.first);
    }
    for (uint8_t i = 0; i < 8; i++)
    {
        for (const auto& kv : *(_levels + i))
        {
            ret.push_back(kv.first);
        }
    }

    return ret;
}

// _____________________________________________________________________________
osm_filter osm_filter::merge(const osm_filter& other) const
{
    multi_attribute_map keep;
    multi_attribute_map drop;

    for (const auto& kv : _keep)
    {
        keep[kv.first].insert(kv.second.begin(), kv.second.end());
    }

    for (const auto& kv : other._keep)
    {
        keep[kv.first].insert(kv.second.begin(), kv.second.end());
    }

    return osm_filter(keep, drop);
}

// _____________________________________________________________________________
std::string osm_filter::toString() const
{
    std::stringstream ss;
    ss << "[KEEP]\n\n";

    for (const auto& kv : _keep)
    {
        ss << " " << kv.first << "=";
        bool first = false;
        for (const auto& v : kv.second)
        {
            if (first) ss << ",";
            first = true;
            ss << v.first;
        }
        ss << "\n";
    }

    ss << "\n[DROP]\n\n";

    for (const auto& kv : _drop)
    {
        ss << " " << kv.first << "=";
        bool first = false;
        for (const auto& v : kv.second)
        {
            if (first) ss << ",";
            first = true;
            ss << v.first;
        }
        ss << "\n";
    }

    return ss.str();
}

// _____________________________________________________________________________
uint64_t osm_filter::negRestr(const attribute_map& attrs) const
{
    if (contained(attrs, _noRestr, ALL)) return false;
    return (contained(attrs, _negRestr, ALL));
}

// _____________________________________________________________________________
uint64_t osm_filter::posRestr(const attribute_map& attrs) const
{
    if (contained(attrs, _noRestr, ALL)) return false;
    return (contained(attrs, _posRestr, ALL));
}

// _____________________________________________________________________________
const pfaedle::osm::multi_attribute_map& osm_filter::getKeepRules() const
{
    return _keep;
}

// _____________________________________________________________________________
const pfaedle::osm::multi_attribute_map& osm_filter::getDropRules() const
{
    return _drop;
}

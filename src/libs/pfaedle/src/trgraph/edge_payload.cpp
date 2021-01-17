// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/edge_payload.h"
#include "util/geo/Geo.h"
#include <map>
#include <string>
#include <vector>

using pfaedle::trgraph::edge_payload;
using pfaedle::trgraph::transit_edge_line;


std::map<LINE*, size_t> edge_payload::_flines;
std::map<const transit_edge_line*, size_t> edge_payload::_tlines;

// _____________________________________________________________________________
edge_payload::edge_payload() :
    _length(0), _max_speed(50.f), _oneWay(0), _hasRestr(false), _rev(false), _lvl(0)
{
    _l = new LINE();
    _flines[_l] = 1;
}

// _____________________________________________________________________________
edge_payload::edge_payload(const edge_payload& pl) :
    edge_payload(pl, false) {}

// _____________________________________________________________________________
edge_payload::edge_payload(const edge_payload& pl, bool geoflat) :
    _length(pl._length),
    _max_speed(pl._max_speed),
    _oneWay(pl._oneWay),
    _hasRestr(pl._hasRestr),
    _rev(pl._rev),
    _lvl(pl._lvl)
{
    if (geoflat)
    {
        _l = pl._l;
    }
    else
    {
        _l = new LINE(*pl._l);
    }
    _flines[_l]++;

    for (auto l : pl._lines) add_line(l);
}

// _____________________________________________________________________________
edge_payload::~edge_payload()
{
    if (_l)
    {
        _flines[_l]--;
        if (_flines[_l] == 0) delete _l;
    }

    for (auto l : _lines) unRefTLine(l);
}

// _____________________________________________________________________________
void edge_payload::unRefTLine(const transit_edge_line* l)
{
    if (l)
    {
        _tlines[l]--;
        if (_tlines[l] == 0)
        {
            delete l;
            _tlines.erase(_tlines.find(l));
        }
    }
}

// _____________________________________________________________________________
edge_payload edge_payload::revCopy() const
{
    edge_payload ret(*this);
    ret.set_reversed();
    if (ret.oneWay() == 1)
        ret.setOneWay(2);
    else if (ret.oneWay() == 2)
        ret.setOneWay(1);
    return ret;
}

// _____________________________________________________________________________
void edge_payload::set_length(double d) { _length = d; }

// _____________________________________________________________________________
double edge_payload::get_length() const { return _length; }

// _____________________________________________________________________________
void edge_payload::set_max_speed(double speed){ _max_speed = speed; }

// _____________________________________________________________________________
double edge_payload::get_max_speed() const { return _max_speed; }

// _____________________________________________________________________________
void edge_payload::add_line(const transit_edge_line* l)
{
    if (std::find(_lines.begin(), _lines.end(), l) == _lines.end())
    {
        _lines.reserve(_lines.size() + 1);
        _lines.push_back(l);
        if (_tlines.count(l))
            _tlines[l]++;
        else
            _tlines[l] = 1;
    }
}

// _____________________________________________________________________________
void edge_payload::add_lines(const std::vector<transit_edge_line*>& l)
{
    for (auto line : l) add_line(line);
}

// _____________________________________________________________________________
const std::vector<const transit_edge_line*>& edge_payload::get_lines() const
{
    return _lines;
}

// _____________________________________________________________________________
void edge_payload::add_point(const POINT& p) { _l->push_back(p); }

// _____________________________________________________________________________
const LINE* edge_payload::get_geom() const { return _l; }

// _____________________________________________________________________________
LINE* edge_payload::get_geom() { return _l; }

// _____________________________________________________________________________
util::json::Dict edge_payload::get_attrs() const
{
    util::json::Dict obj;
    obj["m_length"] = std::to_string(_length);
    obj["maxspeed"] = std::to_string(_max_speed);
    obj["oneway"] = std::to_string(static_cast<int>(_oneWay));
    obj["level"] = std::to_string(_lvl);
    obj["restriction"] = is_restricted() ? "yes" : "no";

    std::stringstream ss;
    bool first = false;

    for (auto* l : _lines)
    {
        if (first) ss << ",";
        ss << l->shortName;
        if (!l->fromStr.empty() || !l->toStr.empty())
        {
            ss << "(" << l->fromStr;
            ss << "->" << l->toStr << ")";
        }
        first = true;
    }

    obj["lines"] = ss.str();
    return obj;
}

// _____________________________________________________________________________
void edge_payload::set_restricted() { _hasRestr = true; }

// _____________________________________________________________________________
bool edge_payload::is_restricted() const { return _hasRestr; }

// _____________________________________________________________________________
uint8_t edge_payload::oneWay() const { return _oneWay; }

// _____________________________________________________________________________
void edge_payload::setOneWay(uint8_t dir) { _oneWay = dir; }

// _____________________________________________________________________________
void edge_payload::setOneWay() { _oneWay = 1; }

// _____________________________________________________________________________
void edge_payload::set_level(uint8_t lvl) { _lvl = lvl; }

// _____________________________________________________________________________
uint8_t edge_payload::level() const { return _lvl; }

// _____________________________________________________________________________
void edge_payload::set_reversed() { _rev = true; }

// _____________________________________________________________________________
bool edge_payload::is_reversed() const { return _rev; }

// _____________________________________________________________________________
const POINT& edge_payload::backHop() const
{
    if (is_reversed())
    {
        return *(++(get_geom()->cbegin()));
    }
    return *(++(get_geom()->crbegin()));
}

// _____________________________________________________________________________
const POINT& edge_payload::frontHop() const
{
    if (is_reversed())
    {
        return *(++(get_geom()->crbegin()));
    }
    return *(++(get_geom()->cbegin()));
}

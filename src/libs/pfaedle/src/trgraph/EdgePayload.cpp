// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "pfaedle/trgraph/EdgePayload.h"
#include "util/geo/Geo.h"
#include <map>
#include <string>
#include <vector>

using pfaedle::trgraph::EdgePayload;
using pfaedle::trgraph::TransitEdgeLine;


std::map<LINE*, size_t> EdgePayload::_flines;
std::map<const TransitEdgeLine*, size_t> EdgePayload::_tlines;

// _____________________________________________________________________________
EdgePayload::EdgePayload() :
    _length(0), _oneWay(0), _hasRestr(false), _rev(false), _lvl(0)
{
    _l = new LINE();
    _flines[_l] = 1;
}

// _____________________________________________________________________________
EdgePayload::EdgePayload(const EdgePayload& pl) :
    EdgePayload(pl, false) {}

// _____________________________________________________________________________
EdgePayload::EdgePayload(const EdgePayload& pl, bool geoflat) :
    _length(pl._length),
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

    for (auto l : pl._lines) addLine(l);
}

// _____________________________________________________________________________
EdgePayload::~EdgePayload()
{
    if (_l)
    {
        _flines[_l]--;
        if (_flines[_l] == 0) delete _l;
    }

    for (auto l : _lines) unRefTLine(l);
}

// _____________________________________________________________________________
void EdgePayload::unRefTLine(const TransitEdgeLine* l)
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
EdgePayload EdgePayload::revCopy() const
{
    EdgePayload ret(*this);
    ret.setRev();
    if (ret.oneWay() == 1)
        ret.setOneWay(2);
    else if (ret.oneWay() == 2)
        ret.setOneWay(1);
    return ret;
}

// _____________________________________________________________________________
void EdgePayload::setLength(double d) { _length = d; }

// _____________________________________________________________________________
double EdgePayload::getLength() const { return _length; }

// _____________________________________________________________________________
void EdgePayload::addLine(const TransitEdgeLine* l)
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
void EdgePayload::addLines(const std::vector<TransitEdgeLine*>& l)
{
    for (auto line : l) addLine(line);
}

// _____________________________________________________________________________
const std::vector<const TransitEdgeLine*>& EdgePayload::getLines() const
{
    return _lines;
}

// _____________________________________________________________________________
void EdgePayload::addPoint(const POINT& p) { _l->push_back(p); }

// _____________________________________________________________________________
const LINE* EdgePayload::getGeom() const { return _l; }

// _____________________________________________________________________________
LINE* EdgePayload::getGeom() { return _l; }

// _____________________________________________________________________________
util::json::Dict EdgePayload::getAttrs() const
{
    util::json::Dict obj;
    obj["m_length"] = std::to_string(_length);
    obj["oneway"] = std::to_string(static_cast<int>(_oneWay));
    obj["level"] = std::to_string(_lvl);
    obj["restriction"] = isRestricted() ? "yes" : "no";

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
void EdgePayload::setRestricted() { _hasRestr = true; }

// _____________________________________________________________________________
bool EdgePayload::isRestricted() const { return _hasRestr; }

// _____________________________________________________________________________
uint8_t EdgePayload::oneWay() const { return _oneWay; }

// _____________________________________________________________________________
void EdgePayload::setOneWay(uint8_t dir) { _oneWay = dir; }

// _____________________________________________________________________________
void EdgePayload::setOneWay() { _oneWay = 1; }

// _____________________________________________________________________________
void EdgePayload::setLvl(uint8_t lvl) { _lvl = lvl; }

// _____________________________________________________________________________
uint8_t EdgePayload::lvl() const { return _lvl; }

// _____________________________________________________________________________
void EdgePayload::setRev() { _rev = true; }

// _____________________________________________________________________________
bool EdgePayload::isRev() const { return _rev; }

// _____________________________________________________________________________
const POINT& EdgePayload::backHop() const
{
    if (isRev())
    {
        return *(++(getGeom()->cbegin()));
    }
    return *(++(getGeom()->crbegin()));
}

// _____________________________________________________________________________
const POINT& EdgePayload::frontHop() const
{
    if (isRev())
    {
        return *(++(getGeom()->crbegin()));
    }
    return *(++(getGeom()->cbegin()));
}

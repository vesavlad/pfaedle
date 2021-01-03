// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GEO_OUTPUT_GEOJSONOUTPUT_H_
#define UTIL_GEO_OUTPUT_GEOJSONOUTPUT_H_

#include <map>
#include <ostream>
#include <string>
#include "util/String.h"
#include "util/geo/Geo.h"
#include "util/json/Writer.h"

namespace util::geo::output
{

class GeoJsonOutput
{
public:
    explicit GeoJsonOutput(std::ostream& str);
    GeoJsonOutput(std::ostream& str, const json::Val& attrs);
    ~GeoJsonOutput();

    template<typename T>
    void print(const Point<T>& p, const json::Val& attrs)
    {
        _wr.obj();
        _wr.keyVal("type", "Feature");

        _wr.key("geometry");
        _wr.obj();
        _wr.keyVal("type", "Point");
        _wr.key("coordinates");
        _wr.arr();
        _wr.val(p.getX());
        _wr.val(p.getY());
        _wr.close();
        _wr.close();
        _wr.key("properties");
        _wr.val(attrs);
        _wr.close();
    }

    template<typename T>
    void print(const Line<T>& l, const json::Val& attrs)
    {
        if (l.empty()) return;
        _wr.obj();
        _wr.keyVal("type", "Feature");

        _wr.key("geometry");
        _wr.obj();
        _wr.keyVal("type", "LineString");
        _wr.key("coordinates");
        _wr.arr();
        for (auto p : l)
        {
            _wr.arr();
            _wr.val(p.getX());
            _wr.val(p.getY());
            _wr.close();
        }
        _wr.close();
        _wr.close();
        _wr.key("properties");
        _wr.val(attrs);
        _wr.close();
    }

    template<typename T>
    void printLatLng(const Point<T>& p, const json::Val& attrs)
    {
        auto projP = util::geo::webMercToLatLng<double>(p.getX(), p.getY());
        print(projP, attrs);
    }

    template<typename T>
    void printLatLng(const Line<T>& l, const json::Val& attrs)
    {
        Line<T> projL;
        for (auto p : l)
            projL.push_back(util::geo::webMercToLatLng<double>(p.getX(), p.getY()));

        print(projL, attrs);
    }

    void flush();

private:
    json::Writer _wr;
};
}

#endif  // UTIL_GEO_OUTPUT_GEOJSONOUTPUT_H_

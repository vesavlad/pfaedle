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
        writter_.obj();
        writter_.keyVal("type", "Feature");

        writter_.key("geometry");
        writter_.obj();
        writter_.keyVal("type", "Point");
        writter_.key("coordinates");
        writter_.arr();
        writter_.val(p.getX());
        writter_.val(p.getY());
        writter_.close();
        writter_.close();
        writter_.key("properties");
        writter_.val(attrs);
        writter_.close();
    }

    template<typename T>
    void print(const Line<T>& l, const json::Val& attrs)
    {
        if (l.empty()) return;
        writter_.obj();
        writter_.keyVal("type", "Feature");

        writter_.key("geometry");
        writter_.obj();
        writter_.keyVal("type", "LineString");
        writter_.key("coordinates");
        writter_.arr();
        for (auto p : l)
        {
            writter_.arr();
            writter_.val(p.getX());
            writter_.val(p.getY());
            writter_.close();
        }
        writter_.close();
        writter_.close();
        writter_.key("properties");
        writter_.val(attrs);
        writter_.close();
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
    json::Writer writter_;
};
}

#endif  // UTIL_GEO_OUTPUT_GEOJSONOUTPUT_H_

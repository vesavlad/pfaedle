// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>
//
#include "util/geo/output/GeoJsonOutput.h"

using namespace util::geo::output;

// _____________________________________________________________________________
GeoJsonOutput::GeoJsonOutput(std::ostream& str) :
    writter_(str, 10, true)
{
    writter_.obj();
    writter_.keyVal("type", "FeatureCollection");
    writter_.key("features");
    writter_.arr();
}

// _____________________________________________________________________________
GeoJsonOutput::GeoJsonOutput(std::ostream& str, const json::Val& attrs) :
    writter_(str, 10, true)
{
    writter_.obj();
    writter_.keyVal("type", "FeatureCollection");
    writter_.key("properties");
    writter_.val(attrs);
    writter_.key("features");
    writter_.arr();
}

// _____________________________________________________________________________
GeoJsonOutput::~GeoJsonOutput() { flush(); }

// _____________________________________________________________________________
void GeoJsonOutput::flush() { writter_.closeAll(); }

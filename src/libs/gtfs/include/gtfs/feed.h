#pragma once
#include <gtfs/agency.h>
#include <gtfs/route.h>
#include <gtfs/trip.h>
#include <gtfs/stop.h>
#include <gtfs/stop_time.h>
#include <gtfs/shape.h>
#include <gtfs/feed_info.h>
#include <gtfs/calendar_item.h>
#include <gtfs/calendar_date.h>
#include <gtfs/frequency.h>

#include <gtfs/access/result.h>

#include <vector>
#include <functional>
#include <map>

namespace pfaedle::gtfs
{

// Main classes for working with GTFS feeds
using agency_map = std::map<Id,agency>;
using stop_map = std::map<Id,stop>;
using route_map = std::map<Id,route>;
using trip_map = std::map<Id,trip>;
using stop_time_map = std::map<std::pair<Id,Id>,stop_time>;
using calendar_map = std::map<Id,calendar_item>;
using calendar_dates_map = std::map<Id,calendar_date>;

//using FareRules = std::map<Id,FareRule>;
//using FareAttributes = std::map<Id,FareAttributesItem>;
using shape_map = std::map<Id,shape>;
using frequency_map = std::map<Id,frequency>;
//using Transfers = std::map<Id,Transfer>;
//using Pathways = std::map<Id,Pathway>;
//using Levels = std::map<Id,Level>;

//// FeedInfo is a unique object and doesn't need a container.
//using Translations = std::map<Id,Translation>;
//using Attributions = std::map<Id,Attribution>;


class feed
{
public:
    feed() = default;

    agency_map agencies;
    stop_map stops;
    route_map routes;
    trip_map trips;
    stop_time_map stop_times;

    calendar_map calendar;
    calendar_dates_map calendar_dates;
//    FareRules fare_rules_;
//    FareAttributes fare_attributes_;
    shape_map shapes;
    frequency_map frequencies;
//    Transfers transfers_;
//    Pathways pathways_;
//    Levels levels_;
//    Translations translations_;
//    Attributions attributions_;
    pfaedle::gtfs::feed_info feed_info;
};
}

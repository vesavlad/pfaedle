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
#include <gtfs/transfer.h>
#include <gtfs/fare_rule.h>
#include <gtfs/fare_attributes_item.h>

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
using stop_time_vector = std::vector<stop_time>;
using calendar_map = std::map<Id,calendar_item>;
using calendar_dates_map = std::map<Id,calendar_date>;

using fare_map = std::map<Id,fare_rule>;
using fare_attributes_map = std::map<Id,fare_attributes_item>;
using shape_map = std::map<Id,shape>;
using frequency_vector = std::vector<frequency>;
using transfer_map = std::map<std::tuple<Id, Id>,transfer>;

class feed;
class provider
{
public:
    virtual ~provider() = default;
    virtual void prepare() = 0;
};

/**
 * @brief class responsible to hold map of stoptimes based on diferrent criterias
 * in order to provide fast access
 */
class stop_time_provider: public provider
{
public:
    explicit stop_time_provider(feed& feed);


    const std::vector<std::reference_wrapper<stop_time>>& get_for_stop(const Id& id) const;
    std::vector<std::reference_wrapper<stop_time>>& get_for_stop(const Id& id);
    const std::vector<std::reference_wrapper<stop_time>>& get_for_trip(const Id& id) const;
    std::vector<std::reference_wrapper<stop_time>>& get_for_trip(const Id& id);

    void prepare() override;

private:
    std::map<Id, std::vector<std::reference_wrapper<stop_time>>> stop_based_map;
    std::map<Id, std::vector<std::reference_wrapper<stop_time>>> trip_based_map;
    feed& feed_;
};

/**
 * @brief class responsible to hold map of routes based on different criterias
 * in order to provide fast access
 */
class routes_provider: public provider
{
public:
    explicit routes_provider(feed& feed);
    void prepare() override;

    std::vector<std::reference_wrapper<pfaedle::gtfs::route>>& get_for_agency(const Id& id);
    const std::vector<std::reference_wrapper<pfaedle::gtfs::route>>& get_for_agency(const Id& id) const;

private:
    std::map<Id, std::vector<std::reference_wrapper<route>>> agency_based_map;
    feed& feed_;
};

/**
 * @brief class responsible to hold map of trips based on different criterias
 * in order to provide fast access
 */
class trips_provider: public provider
{
public:
    explicit trips_provider(feed& feed);
    void prepare() override;

    std::vector<std::reference_wrapper<pfaedle::gtfs::trip>>& get_for_route(const Id& id);
    const std::vector<std::reference_wrapper<pfaedle::gtfs::trip>>& get_for_route(const Id& id) const;

private:
    std::map<Id, std::vector<std::reference_wrapper<trip>>> route_based_map;
    feed& feed_;
};

class feed
{
public:
    feed();

    pfaedle::gtfs::stop_time_provider& stop_time_provider() const;
    pfaedle::gtfs::routes_provider& routes_provider() const;
    pfaedle::gtfs::trips_provider& trips_provider() const;

    void build();
public:
    agency_map agencies;
    stop_map stops;
    route_map routes;
    trip_map trips;
    stop_time_vector stop_times;

    calendar_map calendar;
    calendar_dates_map calendar_dates;

    fare_map fare_rules;
    fare_attributes_map fare_attributes;
    shape_map shapes;
    frequency_vector frequencies;
    transfer_map transfers;
    pfaedle::gtfs::feed_info feed_info;

private:
    mutable pfaedle::gtfs::stop_time_provider stop_time_provider_;
    mutable pfaedle::gtfs::routes_provider routes_provider_;
    mutable pfaedle::gtfs::trips_provider trips_provider_;
};
}

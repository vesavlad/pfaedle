#pragma once
#include <gtfs/enums/trip_access.h>
#include <gtfs/enums/trip_direction_id.h>
#include <gtfs/record.h>
#include <gtfs/types.h>

#include <functional>
#include <optional>
#include <vector>

namespace pfaedle::gtfs
{
struct stop_time;
struct route;
struct shape;
class feed;

struct trip: public record
{
    trip(pfaedle::gtfs::feed& feed):
        record(feed)
    {

    }

    // Required:
    Id route_id;
    Id service_id;
    Id trip_id;

    // Optional:
    Text trip_headsign;
    Text trip_short_name;
    trip_direction_id direction_id = trip_direction_id::DefaultDirection;
    Id block_id;
    Id shape_id;
    trip_access wheelchair_accessible = trip_access::NoInfo;
    trip_access bikes_allowed = trip_access::NoInfo;

    std::optional<std::reference_wrapper<pfaedle::gtfs::route>> route() const;
    std::optional<std::reference_wrapper<pfaedle::gtfs::shape>> shape() const;
    std::vector<std::reference_wrapper<stop_time>>& stop_times() const;
};
}

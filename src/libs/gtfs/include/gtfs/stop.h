#pragma once
#include <gtfs/record.h>
#include <gtfs/types.h>
#include <gtfs/enums/stop_location_type.h>

#include <functional>
#include <vector>

namespace pfaedle::gtfs
{
struct stop_time;
class feed;

struct stop: public record
{
    stop(pfaedle::gtfs::feed& feed);

    // Required:
    Id stop_id;

    // Conditionally required:
    Text stop_name;

    bool coordinates_present = true;
    double stop_lat = 0.0;
    double stop_lon = 0.0;
    Id zone_id;
    Id parent_station;

    // Optional:
    Text stop_code;
    Text stop_desc;
    Text stop_url;
    stop_location_type location_type = stop_location_type::GenericNode;
    Text stop_timezone;
    Text wheelchair_boarding;
    Id level_id;
    Text platform_code;

    const std::vector<std::reference_wrapper<stop_time>>& get_stop_times() const;
    std::vector<std::reference_wrapper<stop_time>>& get_stop_times();
    std::optional<std::reference_wrapper<stop>> get_parent_station() const;

};
}


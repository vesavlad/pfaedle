#pragma once
#include <pfaedle/gtfs/types.h>
#include <pfaedle/gtfs/time.h>
#include <pfaedle/gtfs/stop_time_boarding.h>
#include <pfaedle/gtfs/stop_time_point.h>

#include <functional>

namespace pfaedle::gtfs
{
struct stop;
struct trip;
struct stop_time
{
    // Required:
    Id trip_id;
    Id stop_id;
    size_t stop_sequence = 0;

    // Conditionally required:
    time arrival_time;

    time departure_time;

    // Optional:
    Text stop_headsign;
    stop_time_boarding pickup_type = stop_time_boarding::RegularlyScheduled;
    stop_time_boarding drop_off_type = stop_time_boarding::RegularlyScheduled;

    double shape_dist_traveled = 0.0;
    stop_time_point timepoint = stop_time_point::Exact;

    std::optional<std::reference_wrapper<pfaedle::gtfs::stop>> stop;
    std::optional<std::reference_wrapper<pfaedle::gtfs::trip>> trip;
};
}


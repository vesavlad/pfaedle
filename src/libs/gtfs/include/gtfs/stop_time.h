#pragma once
#include <gtfs/enums/stop_time_boarding.h>
#include <gtfs/enums/stop_time_point.h>
#include <gtfs/record.h>
#include <gtfs/time.h>
#include <gtfs/types.h>

#include <functional>

namespace pfaedle::gtfs
{
struct stop;
struct trip;
class feed;
struct stop_time: public record
{
    stop_time(pfaedle::gtfs::feed& feed):
        record(feed)
    {}

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

    std::optional<std::reference_wrapper<pfaedle::gtfs::stop>> stop() const;
    std::optional<std::reference_wrapper<pfaedle::gtfs::trip>> trip() const;
};
}


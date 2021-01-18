#pragma once
#include <gtfs/time.h>
#include <gtfs/types.h>
#include <gtfs/record.h>
#include <gtfs/enums/frequency_trip_service.h>

namespace pfaedle::gtfs
{
struct trip;
// Optional dataset file
struct frequency: public record
{
    frequency(pfaedle::gtfs::feed& feed):
        record(feed)
    {

    }

    // Required:
    Id trip_id;
    time start_time;
    time end_time;
    size_t headway_secs = 0;

    // Optional:
    frequency_trip_service exact_times = frequency_trip_service::FrequencyBased;

    pfaedle::gtfs::trip& trip() const;
};

}

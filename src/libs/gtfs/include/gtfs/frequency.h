#pragma once
#include <gtfs/types.h>
#include <gtfs/time.h>
#include <gtfs/frequency_trip_service.h>

namespace pfaedle::gtfs
{
// Optional dataset file
struct frequency
{
    // Required:
    Id trip_id;
    time start_time;
    time end_time;
    size_t headway_secs = 0;

    // Optional:
    frequency_trip_service exact_times = frequency_trip_service::FrequencyBased;
};
}

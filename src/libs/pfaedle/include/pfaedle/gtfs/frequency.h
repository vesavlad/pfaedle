#pragma once
#include <pfaedle/gtfs/types.h>
#include <pfaedle/gtfs/time.h>
#include <pfaedle/gtfs/frequency_trip_service.h>

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

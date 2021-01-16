#pragma once
namespace pfaedle::gtfs
{
enum class frequency_trip_service
{
    FrequencyBased = 0,  // Frequency-based trips
    ScheduleBased = 1    // Schedule-based trips with the exact same headway throughout the day
};
}

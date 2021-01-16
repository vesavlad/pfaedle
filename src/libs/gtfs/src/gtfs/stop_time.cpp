#include <gtfs/stop_time.h>
#include <gtfs/feed.h>

namespace pfaedle::gtfs
{
std::optional<std::reference_wrapper<pfaedle::gtfs::stop>> stop_time::stop() const
{
    if(!feed.stops.count(stop_id))
        return std::nullopt;

    return feed.stops.at(stop_id);
}
std::optional<std::reference_wrapper<pfaedle::gtfs::trip>> stop_time::trip() const
{
    if(!feed.trips.count(trip_id))
        return std::nullopt;

    return feed.trips.at(trip_id);
}
}

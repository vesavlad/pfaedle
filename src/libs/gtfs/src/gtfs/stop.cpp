#include <gtfs/stop.h>
#include <gtfs/feed.h>
namespace pfaedle::gtfs
{

stop::stop(pfaedle::gtfs::feed& feed) :
    record(feed)
{
}
const std::vector<std::reference_wrapper<stop_time>>& stop::get_stop_times() const
{
    return feed.stop_time_provider().get_for_stop(stop_id);
}

std::vector<std::reference_wrapper<stop_time>>& stop::get_stop_times()
{
    return feed.stop_time_provider().get_for_stop(stop_id);
}
std::optional<std::reference_wrapper<stop>> stop::get_parent_station() const
{
    if (feed.stops.count(parent_station))
        return feed.stops.at(parent_station);

    return std::nullopt;
}
}

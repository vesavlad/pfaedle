#include <gtfs/trip.h>
#include <gtfs/feed.h>
#include <iostream>
namespace pfaedle::gtfs
{
std::optional<std::reference_wrapper<pfaedle::gtfs::route>> trip::route() const
{
    if(!feed.routes.count(route_id))
        return std::nullopt;

    return feed.routes.at(route_id);
}
std::optional<std::reference_wrapper<pfaedle::gtfs::shape>> trip::shape() const
{
    if(!feed.shapes.count(shape_id))
        return std::nullopt;

    return feed.shapes.at(shape_id);
}
std::vector<std::reference_wrapper<stop_time>>& trip::stop_times() const
{
    auto& provider = feed.stop_time_provider();
    return provider.get_for_trip(trip_id);
}
}

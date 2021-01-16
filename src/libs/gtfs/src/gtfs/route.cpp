#include <gtfs/route.h>
#include <gtfs/feed.h>
namespace pfaedle::gtfs
{
std::optional<std::reference_wrapper<agency>> route::agency() const
{
    if (agency_id.empty() || feed.agencies.count(agency_id))
        return std::nullopt;

    return feed.agencies.at(agency_id);
}
std::vector<std::reference_wrapper<trip>> route::trips() const
{
    return feed.trips_provider().get_for_route(route_id);
}

}

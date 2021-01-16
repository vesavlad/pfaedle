#include <gtfs/feed.h>
namespace pfaedle::gtfs
{

stop_time_provider::stop_time_provider(feed& feed) :
    feed_(feed)
{
}
const std::vector<std::reference_wrapper<stop_time>>& stop_time_provider::get_for_stop(const Id& id) const
{
    return stop_based_map.at(id);
}
std::vector<std::reference_wrapper<stop_time>>& stop_time_provider::get_for_stop(const Id& id)
{
    return stop_based_map.at(id);
}
const std::vector<std::reference_wrapper<stop_time>>& stop_time_provider::get_for_trip(const Id& id) const
{
    return trip_based_map.at(id);
}
std::vector<std::reference_wrapper<stop_time>>& stop_time_provider::get_for_trip(const Id& id)
{
    return trip_based_map.at(id);
}
void stop_time_provider::prepare()
{
    for (auto& st : feed_.stop_times)
    {
        trip_based_map[st.trip_id].push_back(std::ref(st));
        stop_based_map[st.stop_id].push_back(std::ref(st));
    }
}



feed::feed() :
    stop_time_provider_(*this),
    routes_provider_(*this),
    trips_provider_(*this)
{
}
pfaedle::gtfs::stop_time_provider& feed::stop_time_provider() const
{
    return stop_time_provider_;
}
void feed::build()
{
    stop_time_provider_.prepare();
    routes_provider_.prepare();
    trips_provider_.prepare();
}
pfaedle::gtfs::routes_provider& feed::routes_provider() const
{
    return routes_provider_;
}
pfaedle::gtfs::trips_provider& feed::trips_provider() const
{
    return trips_provider_;
}


routes_provider::routes_provider(feed& feed) :
    feed_(feed)
{
}
void routes_provider::prepare()
{
    for (auto& r : feed_.routes)
    {
        agency_based_map[r.second.agency_id].push_back(std::ref(r.second));
    }
}
std::vector<std::reference_wrapper<pfaedle::gtfs::route>>& routes_provider::get_for_agency(const Id& id)
{
    return agency_based_map.at(id);
}
const std::vector<std::reference_wrapper<pfaedle::gtfs::route>>& routes_provider::get_for_agency(const Id& id) const
{
    return agency_based_map.at(id);
}

trips_provider::trips_provider(feed& feed) :
        feed_(feed)
{
}
void trips_provider::prepare()
{
    for (auto& t : feed_.trips)
    {
        route_based_map[t.second.route_id].push_back(std::ref(t.second));
    }
}
std::vector<std::reference_wrapper<pfaedle::gtfs::trip>>& trips_provider::get_for_route(const Id& id)
{
    return route_based_map.at(id);
}
const std::vector<std::reference_wrapper<pfaedle::gtfs::trip>>& trips_provider::get_for_route(const Id& id) const
{
    return route_based_map.at(id);
}
}

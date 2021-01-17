#include <pfaedle/router/misc.h>

namespace pfaedle::router
{
std::string get_mot_str(const pfaedle::router::route_type_set& mots)
{
    bool first = false;
    std::string motStr;
    for (const auto& mot : mots)
    {
        if (first)
        {
            motStr += ", ";
        }
        motStr += "<" + pfaedle::gtfs::get_route_type_string(mot) + ">";
        first = true;
    }

    return motStr;
}

pfaedle::router::feed_stops write_mot_stops(const gtfs::feed& feed, const route_type_set& mots, const std::string& tid)
{
    pfaedle::router::feed_stops ret;
    for (auto& t : feed.trips)
    {
        if (!tid.empty() && t.second.trip_id != tid)
            continue;

        if(t.second.route().has_value() && mots.count(t.second.route().value().get().route_type))
        {
            for (const gtfs::stop_time& st : t.second.stop_times())
            {
                if(!st.stop().has_value())
                    continue;

                const gtfs::stop& stop = st.stop()->get();
                // if the station has type STATION_ENTRANCE, use the parent
                // station for routing. Normally, this should not occur, as
                // this is not allowed in stop_times.txt
                if (stop.location_type == pfaedle::gtfs::stop_location_type::EntranceExit && !stop.parent_station.empty())
                {
                    const auto& parent = stop.get_parent_station();
                    if(parent.has_value())
                        ret[&(parent->get())] = nullptr;
                    else
                        ret[&stop] = nullptr;
                }
                else
                {
                    ret[&stop] = nullptr;
                }
            }
        }
    }
    return ret;
}


route_type_set route_type_section(const route_type_set& a, const route_type_set& b)
{
    route_type_set ret;
    for (auto mot : a)
    {
        if (b.count(mot))
        {
            ret.insert(mot);
        }
    }
    return ret;
}
}

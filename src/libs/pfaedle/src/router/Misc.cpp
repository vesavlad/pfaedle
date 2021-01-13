#include <pfaedle/router/Misc.h>

namespace pfaedle::router
{
std::string getMotStr(const pfaedle::router::MOTs& mots)
{
    bool first = false;
    std::string motStr;
    for (const auto& mot : mots)
    {
        if (first)
        {
            motStr += ", ";
        }
        motStr += "<" + ad::cppgtfs::gtfs::flat::Route::getTypeString(mot) + ">";
        first = true;
    }

    return motStr;
}

pfaedle::router::FeedStops writeMotStops(const gtfs::Feed& feed, const MOTs& mots, const string& tid)
{
    pfaedle::router::FeedStops ret;
    for (auto t : feed.getTrips())
    {
        if (!tid.empty() && t.getId() != tid)
            continue;

        if (mots.count(t.getRoute()->getType()))
        {
            for (auto st : t.getStopTimes())
            {
                // if the station has type STATION_ENTRANCE, use the parent
                // station for routing. Normally, this should not occur, as
                // this is not allowed in stop_times.txt
                if (st.getStop()->getLocationType() == ad::cppgtfs::gtfs::flat::Stop::STATION_ENTRANCE &&
                    st.getStop()->getParentStation())
                {
                    ret[st.getStop()->getParentStation()] = nullptr;
                }
                else
                {
                    ret[st.getStop()] = nullptr;
                }
            }
        }
    }
    return ret;
}

MOTs motISect(const MOTs& a, const MOTs& b)
{
    MOTs ret;
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

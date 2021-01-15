// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_NETGRAPH_EDGEPL_H_
#define PFAEDLE_NETGRAPH_EDGEPL_H_

#include <set>
#include <string>
#include <vector>
namespace pfaedle::gtfs
{
struct trip;
}
namespace pfaedle::netgraph
{
/*
 * A payload class for edges on a network graph - that is a graph
 * that exactly represents a physical public transit network
 */
class edge_payload
{
public:
    edge_payload() = default;
    edge_payload(const LINE& l, const std::set<const pfaedle::gtfs::trip*>& trips) :
        _l(l),
        _trips(trips)
    {
        for (const auto t : _trips)
        {
            if(t->route.has_value())
            {
                gtfs::route& r = t->route->get();
                _routeShortNames.insert(r.route_short_name);
            }
            _tripShortNames.insert(t->trip_short_name);
        }
    }
    const LINE* get_geom() const
    {
        return &_l;
    }

    util::json::Dict get_attrs() const
    {
        util::json::Dict obj;
        obj["num_trips"] = static_cast<int>(_trips.size());
        obj["route_short_names"] = util::json::Array(_routeShortNames.begin(), _routeShortNames.end());
        obj["trip_short_names"] = util::json::Array(_tripShortNames.begin(), _tripShortNames.end());
        return obj;
    }

private:
    LINE _l;
    std::set<const pfaedle::gtfs::trip*> _trips;
    std::set<std::string> _routeShortNames;
    std::set<std::string> _tripShortNames;
};
}  // namespace pfaedle

#endif  // PFAEDLE_NETGRAPH_EDGEPL_H_

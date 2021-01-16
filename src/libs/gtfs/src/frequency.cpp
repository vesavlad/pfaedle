#include <gtfs/frequency.h>
#include <gtfs/feed.h>

namespace pfaedle::gtfs
{
trip& frequency::trip() const
{
    return feed.trips.at(trip_id);
}
}

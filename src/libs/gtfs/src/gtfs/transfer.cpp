#include <gtfs/transfer.h>
#include <gtfs/feed.h>

namespace pfaedle::gtfs
{
stop& transfer::from_stop() const
{
    return feed.stops.at(from_stop_id);
}
stop& transfer::to_stop() const
{
    return feed.stops.at(to_stop_id);
}
}

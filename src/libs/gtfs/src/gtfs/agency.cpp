#include <gtfs/agency.h>
#include <gtfs/feed.h>
namespace pfaedle::gtfs
{
std::vector<std::reference_wrapper<route>> agency::routes() const
{
    return feed.routes_provider().get_for_agency(agency_id);
}
}

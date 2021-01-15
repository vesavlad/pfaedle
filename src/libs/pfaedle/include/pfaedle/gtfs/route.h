#pragma once
#include <pfaedle/gtfs/types.h>
#include <pfaedle/gtfs/route_type.h>

#include <functional>
#include <vector>

namespace pfaedle::gtfs
{
using rt = route_type;
struct trip;
struct agency;

struct route
{
    // Required:
    Id route_id;
    rt route_type = rt::Tram;

    // Conditionally required:
    Id agency_id;
    Text route_short_name;
    Text route_long_name;

    // Optional
    Text route_desc;
    Text route_url;
    Text route_color;
    Text route_text_color;
    size_t route_sort_order = 0;  // Routes with smaller value values should be displayed first

    std::vector<std::reference_wrapper<trip>> trips;
    std::optional<std::reference_wrapper<agency>> referenced_agency;
};
}

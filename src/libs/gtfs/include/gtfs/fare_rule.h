#pragma once
#include <gtfs/types.h>

namespace pfaedle::gtfs
{
// Optional dataset file
struct fare_rule
{
    // Required:
    Id fare_id;

    // Optional:
    Id route_id;
    Id origin_id;
    Id destination_id;
    Id contains_id;
};
}

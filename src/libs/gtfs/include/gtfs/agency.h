#pragma once
#include <gtfs/types.h>
#include <gtfs/record.h>

#include <functional>
#include <vector>

namespace pfaedle::gtfs
{
struct route;

struct agency: public record
{
    agency(pfaedle::gtfs::feed& feed):
        record(feed)
    {}

    // Conditionally optional:
    Id agency_id;

    // Required:
    Text agency_name;
    Text agency_url;
    Text agency_timezone;

    // Optional:
    Text agency_lang;
    Text agency_phone;
    Text agency_fare_url;
    Text agency_email;

    std::vector<std::reference_wrapper<route>> routes() const;
};
}


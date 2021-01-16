#pragma once
#include <gtfs/types.h>
#include <gtfs/record.h>
#include <gtfs/enums/transfer_type.h>

namespace pfaedle::gtfs
{
struct stop;
class feed;
// Optional dataset file
struct transfer: public record
{
public:
    transfer(pfaedle::gtfs::feed& feed):
        record(feed)
    {

    }

    // Required:
    Id from_stop_id;
    Id to_stop_id;
    pfaedle::gtfs::transfer_type transfer_type = transfer_type::Recommended;

    // Optional:
    size_t min_transfer_time = 0;

    stop& from_stop() const;

    stop& to_stop() const;

};
}

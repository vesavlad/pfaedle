#pragma once
#include <gtfs/types.h>
#include <gtfs/transfer_type.h>

namespace pfaedle::gtfs
{
// Optional dataset file
struct transfer
{
    // Required:
    Id from_stop_id;
    Id to_stop_id;
    pfaedle::gtfs::transfer_type transfer_type = transfer_type::Recommended;

    // Optional:
    size_t min_transfer_time = 0;
};
}

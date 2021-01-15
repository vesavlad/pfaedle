#pragma once
#include <pfaedle/gtfs/types.h>
#include <pfaedle/gtfs/calendar_date_exception.h>
#include <pfaedle/gtfs/date.h>

namespace pfaedle::gtfs
{
struct calendar_date
{
    // Required:
    Id service_id;
    pfaedle::gtfs::date date;
    calendar_date_exception exception_type = calendar_date_exception::Added;
};
}

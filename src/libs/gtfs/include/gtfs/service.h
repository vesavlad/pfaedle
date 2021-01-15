#pragma once
#include <gtfs/types.h>
#include <gtfs/calendar_date.h>
#include <gtfs/calendar_item.h>

#include <vector>

namespace pfaedle::gtfs
{
struct service
{
    Id service_id;
    std::vector<std::reference_wrapper<calendar_date>> dates;
    std::vector<std::reference_wrapper<calendar_item>> items;
};
}

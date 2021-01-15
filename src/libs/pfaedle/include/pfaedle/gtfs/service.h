#pragma once
#include <pfaedle/gtfs/types.h>
#include <pfaedle/gtfs/calendar_date.h>
#include <pfaedle/gtfs/calendar_item.h>

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

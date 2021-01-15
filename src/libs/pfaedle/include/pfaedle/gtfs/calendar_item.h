#pragma once
#include <pfaedle/gtfs/types.h>
#include <pfaedle/gtfs/calendar_availability.h>
#include <pfaedle/gtfs/date.h>

namespace pfaedle::gtfs
{
struct calendar_item
{
    // Required:
    Id service_id;

    calendar_availability monday = calendar_availability::NotAvailable;
    calendar_availability tuesday = calendar_availability::NotAvailable;
    calendar_availability wednesday = calendar_availability::NotAvailable;
    calendar_availability thursday = calendar_availability::NotAvailable;
    calendar_availability friday = calendar_availability::NotAvailable;
    calendar_availability saturday = calendar_availability::NotAvailable;
    calendar_availability sunday = calendar_availability::NotAvailable;

    date start_date;
    date end_date;
};
}

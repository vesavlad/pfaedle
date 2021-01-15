#pragma once
#include <gtfs/types.h>
#include <gtfs/date.h>

namespace pfaedle::gtfs
{
struct feed_info
{
    // Required:
    Text feed_publisher_name;
    Text feed_publisher_url;
    LanguageCode feed_lang;

    // Optional:
    date feed_start_date;
    date feed_end_date;
    Text feed_version;
    Text feed_contact_email;
    Text feed_contact_url;
};
}

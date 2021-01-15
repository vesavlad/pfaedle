#pragma once

#include <gtfs/feed.h>
#include <gtfs/access/result_code.h>

namespace pfaedle::gtfs::access
{
using parsed_csv_row = std::map<std::string, std::string>;

class feed_reader
{
public:
    struct read_config
    {
        // Read required files:
        bool agencies = true;
        bool stops = true;
        bool routes = true;
        bool trips = true;
        bool stop_times = true;

        // Read conditionally required files:
        bool calendar = true;
        bool calendar_dates = true;

        // Read optional files:
        bool shapes = true;
        bool transfers = true;
        bool frequencies = true;
        bool fare_attributes = true;
        bool fare_rules = true;
        bool pathways = true;
        bool levels = true;

        bool attributions = true;
        bool feed_info = true;
        bool translations = true;
    };

    feed_reader(feed& feed, const std::string& directory);
    result read(const read_config& config) noexcept;

protected:

    result parse_csv(const std::string& filename,
                     const std::function<result(const parsed_csv_row& record)>& add_entity) noexcept;

    result read_agencies();

    result read_stops();

    result read_routes();

    result read_trips();


    result read_stop_times();

    result read_calendar();


    result read_calendar_dates();

    result read_fare_rules();

    result read_fare_attributes();

    result read_shapes();

    result read_frequencies();


    result read_transfers();

    result read_pathways();


    result read_levels();

    result read_feed_info();

    result read_translations();


    result read_attributions();

private:
    feed& feed_;
    std::string gtfs_directory_;
};
}


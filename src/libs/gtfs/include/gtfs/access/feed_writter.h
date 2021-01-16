#pragma once
#include <string>
#include <gtfs/access/result.h>
#include <functional>
namespace pfaedle::gtfs
{
class feed;
namespace access
{
class feed_writter
{
public:
    struct write_config
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

    feed_writter(feed& feed, const std::string& directory);
    result write(const write_config& config) noexcept;

protected:
    static result write_csv(const std::string & path, const std::string & file,
                                  const std::function<void(std::ofstream & out)> & write_header,
                                  const std::function<void(std::ofstream & out)> & write_entities);

    result write_agencies() const;
    result write_routes() const;
    result write_shapes() const;
    result write_trips() const;
    result write_stops() const;
    result write_stop_times() const;
    result write_calendar() const;
    result write_calendar_dates() const;
    result write_transfers() const;
    result write_frequencies() const;
    result write_fare_attributes() const;
    result write_fare_rules() const;
    result write_feed_info() const;
private:
    feed& feed_;
    std::string gtfs_directory_;
};
}
}

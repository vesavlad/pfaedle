#include <gtfs/access/feed_writter.h>
#include <gtfs/feed.h>
#include <gtfs/misc.h>

#include <fstream>
#include <vector>
#include <string>

namespace pfaedle::gtfs
{
void write_agency_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"agency_id",       "agency_name", "agency_url",
                                       "agency_timezone", "agency_lang", "agency_phone",
                                       "agency_fare_url", "agency_email"};
    write_joined(out, std::move(fields));
}

void write_routes_header(std::ofstream & out)
{
    std::vector<std::string> fields = {
            "route_id",         "agency_id",        "route_short_name",  "route_long_name",
            "route_desc",       "route_type",       "route_url",         "route_color",
            "route_text_color", "route_sort_order", "continuous_pickup", "continuous_drop_off"};
    write_joined(out, std::move(fields));
}

void write_shapes_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"shape_id", "shape_pt_lat", "shape_pt_lon",
                                       "shape_pt_sequence"};
    write_joined(out, std::move(fields));
}

void write_trips_header(std::ofstream & out)
{
    std::vector<std::string> fields = {
            "route_id",     "service_id", "trip_id",  "trip_headsign",         "trip_short_name",
            "direction_id", "block_id",   "shape_id", "wheelchair_accessible", "bikes_allowed"};
    write_joined(out, std::move(fields));
}

void write_stops_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"stop_id",        "stop_code",     "stop_name",
                                       "stop_desc",      "stop_lat",      "stop_lon",
                                       "zone_id",        "stop_url",      "location_type",
                                       "parent_station", "stop_timezone", "wheelchair_boarding",
                                       "level_id",       "platform_code"};
    write_joined(out, std::move(fields));
}

void write_stop_times_header(std::ofstream & out)
{
    std::vector<std::string> fields = {
            "trip_id",           "arrival_time",        "departure_time",      "stop_id",
            "stop_sequence",     "stop_headsign",       "pickup_type",         "drop_off_type",
            "continuous_pickup", "continuous_drop_off", "shape_dist_traveled", "timepoint"};
    write_joined(out, std::move(fields));
}

void write_calendar_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"service_id", "monday",   "tuesday", "wednesday",  "thursday",
                                       "friday",     "saturday", "sunday",  "start_date", "end_date"};
    write_joined(out, std::move(fields));
}

void write_calendar_dates_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"service_id", "date", "exception_type"};
    write_joined(out, std::move(fields));
}

void write_transfers_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"from_stop_id", "to_stop_id", "transfer_type",
                                       "min_transfer_time"};
    write_joined(out, std::move(fields));
}

void write_frequencies_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"trip_id", "start_time", "end_time", "headway_secs",
                                       "exact_times"};
    write_joined(out, std::move(fields));
}

void write_fare_attributes_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"fare_id",   "price",     "currency_type",    "payment_method",
                                       "transfers", "agency_id", "transfer_duration"};
    write_joined(out, std::move(fields));
}

void write_fare_rules_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"fare_id", "route_id", "origin_id", "destination_id",
                                       "contains_id"};
    write_joined(out, std::move(fields));
}

void write_pathways_header(std::ofstream & out)
{
    std::vector<std::string> fields = {
            "pathway_id",       "from_stop_id", "to_stop_id",     "pathway_mode",
            "is_bidirectional", "length",       "traversal_time", "stair_count",
            "max_slope",        "min_width",    "signposted_as",  "reversed_signposted_as"};
    write_joined(out, std::move(fields));
}

void write_levels_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"level_id", "level_index", "level_name"};
    write_joined(out, std::move(fields));
}

void write_feed_info_header(std::ofstream & out)
{
    std::vector<std::string> fields = {
            "feed_publisher_name", "feed_publisher_url", "feed_lang",
            "default_lang",        "feed_start_date",    "feed_end_date",
            "feed_version",        "feed_contact_email", "feed_contact_url"};
    write_joined(out, std::move(fields));
}

void write_translations_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"table_name", "field_name",    "language",   "translation",
                                       "record_id",  "record_sub_id", "field_value"};
    write_joined(out, std::move(fields));
}

void write_attributions_header(std::ofstream & out)
{
    std::vector<std::string> fields = {"attribution_id",    "agency_id",         "route_id",
                                       "trip_id",           "organization_name", "is_producer",
                                       "is_operator",       "is_authority",      "attribution_url",
                                       "attribution_email", "attribution_phone"};
    write_joined(out, std::move(fields));
}

namespace access
{


feed_writter::feed_writter(feed& feed, const std::string& directory) :
    feed_{feed},
    gtfs_directory_{add_trailing_slash(directory)}
{}
result feed_writter::write(const write_config& config) noexcept
{
    // Read required files:
    if (config.agencies)
        if (auto res = write_agencies(); res != result_code::OK)
            return res;

    if (config.stops)
        if (auto res = write_stops(); res != result_code::OK)
            return res;

    if (config.routes)
        if (auto res = write_routes(); res != result_code::OK)
            return res;

    if (config.trips)
        if (auto res = write_trips(); res != result_code::OK)
            return res;

    if (config.stop_times)
        if (auto res = write_stop_times(); res != result_code::OK)
            return res;

    // Read conditionally required files:
    if (config.calendar)
        if (auto res = write_calendar(); res != result_code::OK)
            return res;

    if (config.calendar_dates)
        if (auto res = write_calendar_dates(); res != result_code::OK)
            return res;

    // Read optional files:
    if (config.shapes)
        if (auto res = write_shapes(); res != result_code::OK)
            return res;

    if (config.transfers)
        if (auto res = write_transfers(); res != result_code::OK)
            return res;

    if (config.frequencies)
        if (auto res = write_frequencies(); res != result_code::OK)
            return res;

    if (config.fare_attributes)
        if (auto res = write_fare_attributes(); res != result_code::OK)
            return res;

    if (config.fare_rules)
        if (auto res = write_fare_rules(); res != result_code::OK)
            return res;

    if (config.pathways)
        if (auto res = write_pathways(); res != result_code::OK)
            return res;

    if (config.levels)
        if (auto res = write_levels(); res != result_code::OK)
            return res;

    if (config.attributions)
        if (auto res = write_attributions(); res != result_code::OK)
            return res;

    if (config.feed_info)
        if (auto res = write_feed_info(); res != result_code::OK)
            return res;

    if (config.translations)
        if (auto res = write_translations(); res != result_code::OK)
            return res;

    return result_code::OK;
}

result feed_writter::write_csv(const std::string& path, const std::string& file,
                               const std::function<void(std::ofstream&)>& write_header,
                               const std::function<void(std::ofstream&)>& write_entities)
{
    const std::string filepath = add_trailing_slash(path) + file;
    std::ofstream out(filepath);

    if (!out.is_open())
        return {result_code::ERROR_INVALID_GTFS_PATH, "Could not open path for writing " + filepath};

    write_header(out);
    write_entities(out);
    return result_code::OK;
}
result feed_writter::write_agencies() const
{
    auto container_writer = [this](std::ofstream& out) {
        for (const auto& agency_pair : feed_.agencies)
        {
            const auto& agency = agency_pair.second;
            std::vector<std::string> fields{wrap(agency.agency_id), wrap(agency.agency_name),
                                            wrap(agency.agency_url), agency.agency_timezone,
                                            agency.agency_lang, wrap(agency.agency_phone),
                                            agency.agency_fare_url, agency.agency_email};
            write_joined(out, std::move(fields));
        }
    };
    return write_csv(gtfs_directory_, file_agency, write_agency_header, container_writer);
}
result feed_writter::write_routes() const
{
    auto container_writer = [this](std::ofstream& out) {
        for (const auto& route_pair : feed_.routes)
        {
            const auto& route = route_pair.second;
            std::vector<std::string> fields{wrap(route.route_id),
                                            wrap(route.agency_id),
                                            wrap(route.route_short_name),
                                            wrap(route.route_long_name),
                                            wrap(route.route_desc),
                                            wrap(route.route_type),
                                            route.route_url,
                                            route.route_color,
                                            route.route_text_color,
                                            wrap(route.route_sort_order),
                                            "" /* continuous_pickup */,
                                            "" /* continuous_drop_off */};
            // TODO: handle new route fields.
            write_joined(out, std::move(fields));
        }
    };
    return write_csv(gtfs_directory_, file_routes, write_routes_header, container_writer);
}
result feed_writter::write_shapes() const
{
    auto container_writer = [this](std::ofstream& out) {
        for (const auto& shape_pair : feed_.shapes)
        {
            const auto& shape = shape_pair.second;
            for (const auto& point : shape.points)
            {

                std::vector<std::string> fields{wrap(point.shape_id), wrap(point.shape_pt_lat),
                                                wrap(point.shape_pt_lon), wrap(point.shape_pt_sequence),
                                                wrap(point.shape_dist_traveled)};
                write_joined(out, std::move(fields));
            }
        }
    };
    return write_csv(gtfs_directory_, file_shapes, write_shapes_header, container_writer);
}
result feed_writter::write_trips() const
{
    auto container_writer = [this](std::ofstream& out) {
        for (const auto& trip_pair : feed_.trips)
        {
            const auto& trip = trip_pair.second;
            std::vector<std::string> fields{
                    wrap(trip.route_id), wrap(trip.service_id), wrap(trip.trip_id),
                    wrap(trip.trip_headsign), wrap(trip.trip_short_name), wrap(trip.direction_id),
                    wrap(trip.block_id), wrap(trip.shape_id), wrap(trip.wheelchair_accessible),
                    wrap(trip.bikes_allowed)};
            write_joined(out, std::move(fields));
        }
    };
    return write_csv(gtfs_directory_, file_trips, write_trips_header, container_writer);
}
result feed_writter::write_stops() const
{
    auto container_writer = [this](std::ofstream& out) {
        for (const auto& stop_pair : feed_.stops)
        {
            const auto& stop = stop_pair.second;
            std::vector<std::string> fields{
                    wrap(stop.stop_id), wrap(stop.stop_code), wrap(stop.stop_name),
                    wrap(stop.stop_desc), wrap(stop.stop_lat), wrap(stop.stop_lon),
                    wrap(stop.zone_id), stop.stop_url, wrap(stop.location_type),
                    wrap(stop.parent_station), stop.stop_timezone, wrap(stop.wheelchair_boarding),
                    wrap(stop.level_id), wrap(stop.platform_code)};
            write_joined(out, std::move(fields));
        }
    };
    return write_csv(gtfs_directory_, file_stops, write_stops_header, container_writer);
}
result feed_writter::write_stop_times() const
{
    auto container_writer = [this](std::ofstream& out) {
        for (const auto& stop_time_pair : feed_.stop_times)
        {
            const auto& stop_time = stop_time_pair.second;
            std::vector<std::string> fields{wrap(stop_time.trip_id),
                                            stop_time.arrival_time.get_raw_time(),
                                            stop_time.departure_time.get_raw_time(),
                                            wrap(stop_time.stop_id),
                                            wrap(stop_time.stop_sequence),
                                            wrap(stop_time.stop_headsign),
                                            wrap(stop_time.pickup_type),
                                            wrap(stop_time.drop_off_type),
                                            "" /* continuous_pickup */,
                                            "" /* continuous_drop_off */,
                                            wrap(stop_time.shape_dist_traveled),
                                            wrap(stop_time.timepoint)};
            // TODO: handle new stop_times fields.
            write_joined(out, std::move(fields));
        }
    };
    return write_csv(gtfs_directory_, file_stops, write_stops_header, container_writer);
}
result feed_writter::write_calendar() const {}
result feed_writter::write_calendar_dates() const {}
result feed_writter::write_transfers() const {}
result feed_writter::write_frequencies() const {}
result feed_writter::write_fare_attributes() const {}
result feed_writter::write_fare_rules() const {}
result feed_writter::write_pathways() const {}
result feed_writter::write_levels() const {}
result feed_writter::write_feed_info() const {}
result feed_writter::write_translations() const {}
result feed_writter::write_attributions() const {}
}

}

#pragma once
#include <pfaedle/gtfs/exceptions.h>
#include <pfaedle/gtfs/route_type.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <type_traits>
#include <iomanip>
#include <algorithm>
#include <set>

namespace pfaedle::gtfs
{

// File names and other entities defined in GTFS----------------------------------------------------
const std::string file_agency = "agency.txt";
const std::string file_stops = "stops.txt";
const std::string file_routes = "routes.txt";
const std::string file_trips = "trips.txt";
const std::string file_stop_times = "stop_times.txt";
const std::string file_calendar = "calendar.txt";
const std::string file_calendar_dates = "calendar_dates.txt";
const std::string file_fare_attributes = "fare_attributes.txt";
const std::string file_fare_rules = "fare_rules.txt";
const std::string file_shapes = "shapes.txt";
const std::string file_frequencies = "frequencies.txt";
const std::string file_transfers = "transfers.txt";
const std::string file_pathways = "pathways.txt";
const std::string file_levels = "levels.txt";
const std::string file_feed_info = "feed_info.txt";
const std::string file_translations = "translations.txt";
const std::string file_attributions = "attributions.txt";

constexpr char csv_separator = ',';
constexpr char quote = '"';

std::string append_leading_zero(const std::string& s, bool check = true);

std::string add_trailing_slash(const std::string & path);

void write_joined(std::ofstream & out, std::vector<std::string> && elements);

std::string quote_text(const std::string & text);

std::string unquote_text(const std::string & text);

// Csv field values that contain quotation marks or commas must be enclosed within quotation marks.
std::string wrap(const std::string & text);

// Save to csv coordinates with custom precision.
std::string wrap(double val);

// Save to csv enum value as unsigned integer.
template <typename T>
std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, std::string> wrap(const T & val)
{
    return std::to_string(static_cast<int>(val));
}

std::string trim_spaces(const std::string & token);

std::string normalize(std::string & token, bool has_quotes);





std::string get_hex_color_string(uint32_t color);

std::string get_route_type_string(route_type t);

route_type get_route_type(int t);

std::set<route_type> get_route_types_from_string(std::string name);






#if 0

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
#endif




}

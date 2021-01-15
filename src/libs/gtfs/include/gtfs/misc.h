#pragma once
#include <gtfs/exceptions.h>
#include <gtfs/route_type.h>
#include <string>
#include <vector>
#include <type_traits>
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


}

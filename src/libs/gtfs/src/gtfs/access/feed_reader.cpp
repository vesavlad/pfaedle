#include <gtfs/access/feed_reader.h>
#include "csv_parser.h"
#include <string>
#include <stdexcept>
#include <map>

namespace pfaedle::gtfs::access
{

std::string get_value_or_default(const parsed_csv_row& container, const std::string& key,
                                 const std::string& default_value = "")
{
    const auto it = container.find(key);
    if (it == container.end())
        return default_value;

    return it->second;
}

template<class T>
void set_field(T& field, const parsed_csv_row& container, const std::string& key, bool is_optional = true)
{
    const std::string key_str = get_value_or_default(container, key);
    if (!key_str.empty() || !is_optional)
        field = static_cast<T>(std::stoi(key_str));
}

bool set_fractional(double& field, const parsed_csv_row& container, const std::string& key,
                    bool is_optional = true)
{
    const std::string key_str = get_value_or_default(container, key);
    if (!key_str.empty() || !is_optional)
    {
        field = std::stod(key_str);
        return true;
    }
    return false;
}

// Throw if not valid WGS84 decimal degrees.
void check_coordinates(double latitude, double longitude)
{
    if (latitude < -90.0 || latitude > 90.0)
        throw std::out_of_range("Latitude");

    if (longitude < -180.0 || longitude > 180.0)
        throw std::out_of_range("Longitude");
}
}
namespace pfaedle::gtfs::access
{



feed_reader::feed_reader(feed& feed, const std::string& directory) :
    feed_(feed),
    gtfs_directory_(add_trailing_slash(directory))
{

}

bool error_parsing_optional_file(const result & res)
{
    return res != result_code::OK && res != result_code::ERROR_FILE_ABSENT;
}

result feed_reader::read(const read_config& config) noexcept
{
    // Read required files:
    if (config.agencies)
        if (auto res = read_agencies(); res != result_code::OK)
            return res;

    if (config.stops)
        if (auto res = read_stops(); res != result_code::OK)
            return res;

    if (config.routes)
        if (auto res = read_routes(); res != result_code::OK)
            return res;

    if (config.trips)
        if (auto res = read_trips(); res != result_code::OK)
            return res;

    if (config.stop_times)
        if (auto res = read_stop_times(); res != result_code::OK)
            return res;

    // Read conditionally required files:
    if (config.calendar)
        if (auto res = read_calendar(); error_parsing_optional_file(res))
            return res;

    if (config.calendar_dates)
        if (auto res = read_calendar_dates(); error_parsing_optional_file(res))
            return res;

    // Read optional files:
    if (config.shapes)
        if (auto res = read_shapes(); error_parsing_optional_file(res))
            return res;

    if (config.transfers)
        if (auto res = read_transfers(); error_parsing_optional_file(res))
            return res;

    if (config.frequencies)
        if (auto res = read_frequencies(); error_parsing_optional_file(res))
            return res;

    if (config.fare_attributes)
        if (auto res = read_fare_attributes(); error_parsing_optional_file(res))
            return res;

    if (config.fare_rules)
        if (auto res = read_fare_rules(); error_parsing_optional_file(res))
            return res;

    if (config.pathways)
        if (auto res = read_pathways(); error_parsing_optional_file(res))
            return res;

    if (config.levels)
        if (auto res = read_levels(); error_parsing_optional_file(res))
            return res;

    if (config.attributions)
        if (auto res = read_attributions(); error_parsing_optional_file(res))
            return res;

    if (config.feed_info)
        if (auto res = read_feed_info(); error_parsing_optional_file(res))
            return res;

    if (config.translations)
        if (auto res = read_translations(); error_parsing_optional_file(res))
            return res;

    return result_code::OK;
}

result pfaedle::gtfs::access::feed_reader::parse_csv(const std::string& filename, const std::function<result(const parsed_csv_row&)>& add_entity) noexcept
{
    csv_parser parser(gtfs_directory_);
    auto res_header = parser.read_header(filename);
    if (res_header.code != result_code::OK)
        return res_header;

    parsed_csv_row record;
    result res_row;
    while ((res_row = parser.read_row(record)) != result_code::END_OF_FILE)
    {
        if (res_row != result_code::OK)
            return res_row;

        if (record.empty())
            continue;

        result res = add_entity(record);
        if (res != result_code::OK)
        {
            res.message += " while adding item from " + filename;
            return res;
        }
    }

    return {result_code::OK, {"Parsed " + filename}};
}
result feed_reader::read_agencies()
{
    int index = 0;
    auto handler = [this, &index](const parsed_csv_row& row) -> result {
      agency agency(feed_);

      // Conditionally required id:
      agency.agency_id = get_value_or_default(row, "agency_id", std::to_string(index));

      // Required fields:
      try
      {
          agency.agency_name = row.at("agency_name");
          agency.agency_url = row.at("agency_url");
          agency.agency_timezone = row.at("agency_timezone");
      }
      catch (const std::out_of_range & ex)
      {
          return result{result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }

      // Optional fields:
      agency.agency_lang = get_value_or_default(row, "agency_lang");
      agency.agency_phone = get_value_or_default(row, "agency_phone");
      agency.agency_fare_url = get_value_or_default(row, "agency_fare_url");
      agency.agency_email = get_value_or_default(row, "agency_email");

      feed_.agencies.insert(std::make_pair(agency.agency_id, std::move(agency)));
      return result{result_code::OK};
    };
    return parse_csv(file_agency, handler);
}
result feed_reader::read_stops()
{
    auto handler = [this](const parsed_csv_row& row) ->result {
      stop s;

      try
      {
          s.stop_id = row.at("stop_id");

          // Optional:
          bool const set_lon = set_fractional(s.stop_lon, row, "stop_lon");
          bool const set_lat = set_fractional(s.stop_lat, row, "stop_lat");

          if (!set_lon || !set_lat)
              s.coordinates_present = false;
      }
      catch (const std::out_of_range & ex)
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }
      catch (const std::invalid_argument & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }

      // Conditionally required:
      s.stop_name = get_value_or_default(row, "stop_name");
      s.parent_station = get_value_or_default(row, "parent_station");
      s.zone_id = get_value_or_default(row, "zone_id");

      // Optional:
      s.stop_code = get_value_or_default(row, "stop_code");
      s.stop_desc = get_value_or_default(row, "stop_desc");
      s.stop_url = get_value_or_default(row, "stop_url");
      set_field(s.location_type, row, "location_type");
      s.stop_timezone = get_value_or_default(row, "stop_timezone");
      s.wheelchair_boarding = get_value_or_default(row, "wheelchair_boarding");
      s.level_id = get_value_or_default(row, "level_id");
      s.platform_code = get_value_or_default(row, "platform_code");

      feed_.stops.emplace(s.stop_id, s);

      return result_code::OK;

    };
    return parse_csv(file_stops, handler);
}
result feed_reader::read_routes()
{
    auto handler = [this](const parsed_csv_row& row) -> result {
      route r;

      try
      {
          // Required fields:
          r.route_id = row.at("route_id");
          set_field(r.route_type, row, "route_type", false);

          // Optional:
          set_field(r.route_sort_order, row, "route_sort_order");
      }
      catch (const std::out_of_range & ex)
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }
      catch (const std::invalid_argument & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }

      // Conditionally required:
      r.agency_id = get_value_or_default(row, "agency_id");

      r.route_short_name = get_value_or_default(row, "route_short_name");
      r.route_long_name = get_value_or_default(row, "route_long_name");

      if (r.route_short_name.empty() && r.route_long_name.empty())
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT,
                  "'route_short_name' or 'route_long_name' must be specified"};
      }

      r.route_color = get_value_or_default(row, "route_color");
      r.route_text_color = get_value_or_default(row, "route_text_color");
      r.route_desc = get_value_or_default(row, "route_desc");
      r.route_url = get_value_or_default(row, "route_url");

      auto& agency = feed_.agencies.at(r.agency_id);
      r.referenced_agency = std::ref(agency);

      const auto& it = feed_.routes.emplace(r.route_id, r);
      agency.routes.push_back(std::ref(it.first->second));



      return result_code::OK;
    };
    return parse_csv(file_routes, handler);
}
result feed_reader::read_trips()
{
    auto handler = [this](const parsed_csv_row& row) -> result {
      trip t;
      try
      {
          // Required:
          t.route_id = row.at("route_id");
          t.service_id = row.at("service_id");
          t.trip_id = row.at("trip_id");

          // Optional:
          set_field(t.direction_id, row, "direction_id");
          set_field(t.wheelchair_accessible, row, "wheelchair_accessible");
          set_field(t.bikes_allowed, row, "bikes_allowed");
      }
      catch (const std::out_of_range & ex)
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }
      catch (const std::invalid_argument & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }

      // Optional:
      t.shape_id = get_value_or_default(row, "shape_id");
      t.trip_headsign = get_value_or_default(row, "trip_headsign");
      t.trip_short_name = get_value_or_default(row, "trip_short_name");
      t.block_id = get_value_or_default(row, "block_id");

      t.route = std::ref(feed_.routes[t.route_id]);

      const auto& it = feed_.trips.emplace(t.trip_id, t);
      feed_.routes[t.route_id].trips.push_back(std::ref(it.first->second));

      return result_code::OK;
    };
    return parse_csv(file_trips, handler);
}
result feed_reader::read_stop_times()
{
    auto handler = [this](const parsed_csv_row& row) -> result{
      stop_time st;

      try
      {
          // Required:
          st.trip_id = row.at("trip_id");
          st.stop_id = row.at("stop_id");
          st.stop_sequence = std::stoi(row.at("stop_sequence"));

          // Conditionally required:
          st.departure_time = time(row.at("departure_time"));
          st.arrival_time = time(row.at("arrival_time"));

          // Optional:
          set_field(st.pickup_type, row, "pickup_type");
          set_field(st.drop_off_type, row, "drop_off_type");

          set_fractional(st.shape_dist_traveled, row, "shape_dist_traveled");
          if (st.shape_dist_traveled < 0.0)
              throw std::invalid_argument("Invalid shape_dist_traveled");

          set_field(st.timepoint, row, "timepoint");
      }
      catch (const std::out_of_range & ex)
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }
      catch (const std::invalid_argument & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }
      catch (const invalid_field_format & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }

      // Optional fields:
      st.stop_headsign = get_value_or_default(row, "stop_headsign");
      st.trip = std::ref(feed_.trips[st.trip_id]);
      st.stop = std::ref(feed_.stops[st.stop_id]);

      const auto& pair = feed_.stop_times.emplace(std::make_pair(st.stop_id,st.trip_id), st);

      feed_.trips[st.trip_id].stop_time_list.push_back(std::ref(pair.first->second));
      feed_.stops[st.stop_id].stop_time_list.push_back(std::ref(pair.first->second));

      return result_code::OK;
    };
    return parse_csv(file_stop_times, handler);
}
result feed_reader::read_calendar()
{
    auto handler = [this](const parsed_csv_row& row)->result {
      pfaedle::gtfs::calendar_item calendar_item;
      try
      {
          // Required fields:
          calendar_item.service_id = row.at("service_id");

          set_field(calendar_item.monday, row, "monday", false);
          set_field(calendar_item.tuesday, row, "tuesday", false);
          set_field(calendar_item.wednesday, row, "wednesday", false);
          set_field(calendar_item.thursday, row, "thursday", false);
          set_field(calendar_item.friday, row, "friday", false);
          set_field(calendar_item.saturday, row, "saturday", false);
          set_field(calendar_item.sunday, row, "sunday", false);

          calendar_item.start_date = date(row.at("start_date"));
          calendar_item.end_date = date(row.at("end_date"));
      }
      catch (const std::out_of_range & ex)
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }
      catch (const std::invalid_argument & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }
      catch (const invalid_field_format & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }

      feed_.calendar.emplace(calendar_item.service_id, calendar_item);
      return result_code::OK;
    };
    return parse_csv(file_calendar, handler);
}
result feed_reader::read_calendar_dates()
{
    auto handler = [this](const parsed_csv_row& row) -> result {
      pfaedle::gtfs::calendar_date calendar_date;
      try
      {
          // Required fields:
          calendar_date.service_id = row.at("service_id");

          set_field(calendar_date.exception_type, row, "exception_type", false);
          calendar_date.date = date(row.at("date"));
      }
      catch (const std::out_of_range & ex)
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }
      catch (const std::invalid_argument & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }
      catch (const invalid_field_format & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }

      feed_.calendar_dates.emplace(calendar_date.service_id, calendar_date);
      return result_code::OK;
    };
    return parse_csv(file_calendar_dates, handler);
}
result feed_reader::read_fare_rules()
{
    auto handler = [this](const parsed_csv_row& row) -> result {
        pfaedle::gtfs::fare_rule fare_rule;
        try
        {
            // Required fields:
            fare_rule.fare_id = row.at("fare_id");
        }
        catch (const std::out_of_range& ex)
        {
            return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
        }
        catch (const std::invalid_argument& ex)
        {
            return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
        }
        catch (const invalid_field_format& ex)
        {
            return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
        }

        // Optional fields:
        fare_rule.route_id = get_value_or_default(row, "route_id");
        fare_rule.origin_id = get_value_or_default(row, "origin_id");
        fare_rule.destination_id = get_value_or_default(row, "destination_id");
        fare_rule.contains_id = get_value_or_default(row, "contains_id");

        feed_.fare_rules.emplace(fare_rule.fare_id, fare_rule);

        return result_code::OK;
    };
    return parse_csv(file_fare_rules, handler);
}
result feed_reader::read_fare_attributes()
{
    auto handler = [this](const parsed_csv_row& row) -> result {
        fare_attributes_item item;
        try
        {
            // Required fields:
            item.fare_id = row.at("fare_id");
            set_fractional(item.price, row, "price", false);

            item.currency_type = row.at("currency_type");
            set_field(item.payment_method, row, "payment_method", false);
            set_field(item.transfers, row, "transfers", false);

            // Conditionally optional:
            item.agency_id = get_value_or_default(row, "agency_id");
            set_field(item.transfer_duration, row, "transfer_duration");
        }
        catch (const std::out_of_range& ex)
        {
            return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
        }
        catch (const std::invalid_argument& ex)
        {
            return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
        }
        catch (const invalid_field_format& ex)
        {
            return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
        }

        feed_.fare_attributes.emplace(item.fare_id, item);
        return result_code::OK;
    };
    return parse_csv(file_fare_attributes, handler);
}
result feed_reader::read_shapes()
{
    auto handler = [this](const parsed_csv_row& row) -> result {
      shape_point pt;
      try
      {
          // Required:
          pt.shape_id = row.at("shape_id");
          pt.shape_pt_sequence = std::stoi(row.at("shape_pt_sequence"));

          pt.shape_pt_lon = std::stod(row.at("shape_pt_lon"));
          pt.shape_pt_lat = std::stod(row.at("shape_pt_lat"));
          check_coordinates(pt.shape_pt_lat, pt.shape_pt_lon);

          // Optional:
          set_fractional(pt.shape_dist_traveled, row, "shape_dist_traveled");
          if (pt.shape_dist_traveled < 0.0)
              throw std::invalid_argument("Invalid shape_dist_traveled");
      }
      catch (const std::out_of_range & ex)
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }
      catch (const std::invalid_argument & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }

      auto& shape = feed_.shapes[pt.shape_id];
      if(shape.points.empty())
      {
          shape.shape_id = pt.shape_id;
          const auto& it = std::find_if(std::begin(feed_.trips), std::end(feed_.trips), [&shape](const std::pair<Id, trip>& trip_pair) {
              return trip_pair.second.shape_id == shape.shape_id;
          });
          if (it != std::end(feed_.trips))
          {
              it->second.shape = shape;
          }
      }
      shape.points.emplace_back(pt);

      return result_code::OK;
    };
    return parse_csv(file_shapes, handler);
}
result feed_reader::read_frequencies()
{
    auto handler = [this](const parsed_csv_row& row) -> result{
      pfaedle::gtfs::frequency frequency;
      try
      {
          // Required fields:
          frequency.trip_id = row.at("trip_id");
          frequency.start_time = time(row.at("start_time"));
          frequency.end_time = time(row.at("end_time"));
          set_field(frequency.headway_secs, row, "headway_secs", false);

          // Optional:
          set_field(frequency.exact_times, row, "exact_times");
      }
      catch (const std::out_of_range & ex)
      {
          return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
      }
      catch (const std::invalid_argument & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }
      catch (const invalid_field_format & ex)
      {
          return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
      }

      feed_.frequencies.emplace(frequency.trip_id, frequency);
      return result_code::OK;
    };
    return parse_csv(file_frequencies, handler);
}
result feed_reader::read_transfers()
{
    auto handler = [this](const parsed_csv_row& row) -> result {
        pfaedle::gtfs::transfer transfer;
        try
        {
            // Required fields:
            transfer.from_stop_id = row.at("from_stop_id");
            transfer.to_stop_id = row.at("to_stop_id");
            set_field(transfer.transfer_type, row, "transfer_type", false);

            // Optional:
            set_field(transfer.min_transfer_time, row, "min_transfer_time");
        }
        catch (const std::out_of_range& ex)
        {
            return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
        }
        catch (const std::invalid_argument& ex)
        {
            return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
        }
        catch (const invalid_field_format& ex)
        {
            return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
        }

        feed_.transfers.emplace(std::make_tuple(transfer.from_stop_id, transfer.to_stop_id), transfer);
        return result_code::OK;
    };
    return parse_csv(file_transfers, handler);
}
result feed_reader::read_pathways()
{
//    auto handler = [this](const parsed_csv_row& record) { return feed_.add_pathway(record); };
//    return parse_csv(file_pathways, handler);
    return result();
}
result feed_reader::read_levels()
{
//    auto handler = [this](const parsed_csv_row& record) { return feed_.add_level(record); };
//    return parse_csv(file_levels, handler);
    return result();
}
result feed_reader::read_feed_info()
{
    auto handler = [this](const parsed_csv_row& row) -> result {
        try
        {
            // Required fields:
            feed_.feed_info.feed_publisher_name = row.at("feed_publisher_name");
            feed_.feed_info.feed_publisher_url = row.at("feed_publisher_url");
            feed_.feed_info.feed_lang = row.at("feed_lang");

            // Optional fields:
            feed_.feed_info.feed_start_date = date(get_value_or_default(row, "feed_start_date"));
            feed_.feed_info.feed_end_date = date(get_value_or_default(row, "feed_end_date"));
        }
        catch (const std::out_of_range& ex)
        {
            return {result_code::ERROR_REQUIRED_FIELD_ABSENT, ex.what()};
        }
        catch (const std::invalid_argument& ex)
        {
            return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
        }
        catch (const invalid_field_format& ex)
        {
            return {result_code::ERROR_INVALID_FIELD_FORMAT, ex.what()};
        }

        // Optional fields:
        feed_.feed_info.feed_version = get_value_or_default(row, "feed_version");
        feed_.feed_info.feed_contact_email = get_value_or_default(row, "feed_contact_email");
        feed_.feed_info.feed_contact_url = get_value_or_default(row, "feed_contact_url");

        return result_code::OK;
    };
    return parse_csv(file_feed_info, handler);
}
result feed_reader::read_translations()
{
//    auto handler = [this](const parsed_csv_row& record) { return feed_.add_translation(record); };
//    return parse_csv(file_translations, handler);
    return result();
}
result feed_reader::read_attributions()
{
//    auto handler = [this](const parsed_csv_row& record) { return feed_.add_attribution(record); };
//    return parse_csv(file_attributions, handler);
    return result();
}


}

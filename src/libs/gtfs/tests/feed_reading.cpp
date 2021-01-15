#include "catch_amalgamated.hpp"

#include <gtfs/access/feed_reader.h>
#include <gtfs/access/feed_writter.h>
#include <gtfs/feed.h>
#include "config.h"


using namespace pfaedle::gtfs;

// Credits:
// https://developers.google.com/transit/gtfs/examples/gtfs-feed
TEST_CASE("Empty container before parsing")
{
    feed feed;
    access::feed_reader reader(feed,"data/non_existing_dir");
    REQUIRE(feed.agencies.empty());
    auto agency = feed.agencies.count("agency_10");
    CHECK(!agency);
}

TEST_CASE("Non existend directory")
{
    feed feed;
    access::feed_reader reader(feed,"data/non_existing_dir");
    access::feed_reader::read_config config{false, true, false, false, false, false ,false,
    false, false, false, false, false, false ,false, false ,false, false};

    REQUIRE(reader.read(config) == access::result_code::ERROR_FILE_ABSENT);
    REQUIRE(feed.stops.empty());
}

TEST_CASE("Transfers")
{
//    feed feed;
//    access::feed_reader reader(feed,TEST_FOLDER_PATH"/resources/sample_feed");
//    access::feed_reader::read_config config{false, false, false, false, false, false ,false,
//                                            false, true, false, false, false, false ,false, false ,false, false};
//
//    REQUIRE(reader.read(config) == access::result_code::OK);
//    const auto & transfers = feed.transfers;
//    CHECK_EQ(transfers.size(), 4);
//
//    CHECK_EQ(transfers[0].from_stop_id, "130");
//    CHECK_EQ(transfers[0].to_stop_id, "4");
//    CHECK_EQ(transfers[0].transfer_type, TransferType::MinimumTime);
//    CHECK_EQ(transfers[0].min_transfer_time, 70);
//
//    const auto & transfer = feed.get_transfer("314", "11");
//    REQUIRE(transfer);
//    CHECK_EQ(transfer.value().transfer_type, TransferType::Timed);
//    CHECK_EQ(transfer.value().min_transfer_time, 0);
}

TEST_CASE("Calendar")
{
    feed feed;
    access::feed_reader reader(feed,TEST_FOLDER_PATH"/resources/sample_feed");
    access::feed_reader::read_config config{false, false, false, false, false, true ,false,
                                            false, false, false, false, false, false ,false, false ,false, false};

    REQUIRE(reader.read(config) == access::result_code::OK);
    const auto & calendar = feed.calendar;
    REQUIRE(calendar.size() == 2);

    REQUIRE(feed.calendar.count("WE"));

    const auto & calendar_record = feed.calendar.at("WE");
    CHECK(calendar_record.start_date == date(2007, 01, 01));
    CHECK(calendar_record.end_date == date(2010, 12, 31));

    CHECK(calendar_record.monday == calendar_availability::NotAvailable);
    CHECK(calendar_record.tuesday == calendar_availability::NotAvailable);
    CHECK(calendar_record.wednesday == calendar_availability::NotAvailable);
    CHECK(calendar_record.thursday == calendar_availability::NotAvailable);
    CHECK(calendar_record.friday == calendar_availability::NotAvailable);
    CHECK(calendar_record.saturday == calendar_availability::Available);
    CHECK(calendar_record.sunday == calendar_availability::Available);
}

TEST_CASE("Calendar dates")
{
    feed feed;
    access::feed_reader reader(feed,TEST_FOLDER_PATH"/resources/sample_feed");
    access::feed_reader::read_config config{false, false, false, false, false, false ,true,
                                            false, false, false, false, false, false ,false, false ,false, false};

    REQUIRE(reader.read(config) == access::result_code::OK);
    REQUIRE(feed.calendar_dates.size() == 1);

    const auto & calendar_record = feed.calendar_dates.at("FULLW");

    CHECK(calendar_record.date == date(2007, 06, 04));
    CHECK(calendar_record.exception_type == calendar_date_exception::Removed);
}

TEST_CASE("Read GTFS feed")
{
    feed feed;
    access::feed_reader reader(feed,TEST_FOLDER_PATH"/resources/sample_feed");
    access::feed_reader::read_config config;
    REQUIRE(reader.read(config) == access::result_code::OK);

    CHECK(feed.agencies.size() == 1);
    CHECK(feed.routes.size() == 5);
    CHECK(feed.trips.size() == 11);
    CHECK(feed.shapes.size() == 8);
    CHECK(feed.stops.size() == 9);
    CHECK(feed.stop_times.size() == 28);
//    CHECK(feed.get_transfers().size() == 4);
    CHECK(feed.frequencies.size() == 11);
//    CHECK(feed.get_attributions().size() == 1);
    CHECK(feed.calendar.size() == 2);
    CHECK(feed.calendar_dates.size() == 1);
//    CHECK(feed.get_fare_attributes().size() == 2);
//    CHECK(feed.get_fare_rules().size() == 4);
    CHECK(!feed.feed_info.feed_publisher_name.empty());
//    CHECK(feed.get_levels().size() == 3);
//    CHECK(feed.get_pathways().size() == 3);
//    CHECK(feed.get_translations().size() == 1);
}

TEST_CASE("Agency")
{
    pfaedle::gtfs::feed feed;
    access::feed_reader reader(feed,TEST_FOLDER_PATH"/resources/sample_feed");
    access::feed_reader::read_config config{true, false, false, false, false, false , false,
                                            false, false, false, false, false, false ,
                                            false, false ,false, false};

    REQUIRE(reader.read(config) == access::result_code::OK);

    const auto & agencies = feed.agencies;
    REQUIRE(agencies.size() == 1);

    CHECK(agencies.begin()->second.agency_id == "DTA");
    CHECK(agencies.begin()->second.agency_name == "Demo Transit Authority");
    CHECK(agencies.begin()->second.agency_url == "http://google.com");
    CHECK(agencies.begin()->second.agency_lang.empty());
    CHECK(agencies.begin()->second.agency_timezone == "America/Los_Angeles");

    const auto& agency = feed.agencies.at("DTA");

    access::feed_writter writter(feed, TEST_FOLDER_PATH"/resources/output_feed");
    access::feed_writter::write_config writeConfig{true, false, false, false, false, false , false,
                                                   false, false, false, false, false, false ,
                                                   false, false ,false, false};

    REQUIRE(writter.write(writeConfig) == access::result_code::OK);

    pfaedle::gtfs::feed feed_copy;
    access::feed_reader feed_reader_copy(feed_copy,TEST_FOLDER_PATH"/resources/output_feed");
    REQUIRE(feed_reader_copy.read(config) == access::result_code::OK);
    CHECK(agencies.size() == feed_copy.agencies.size());
}
#if 0
TEST_CASE("Routes")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_routes() == access::result_code::OK);

    const auto & routes = feed.get_routes();
    REQUIRE(routes.size() == 5);
    CHECK(routes[0].route_id == "AB");
    CHECK(routes[0].agency_id == "DTA");
    CHECK(routes[0].route_short_name == "10");
    CHECK(routes[0].route_long_name == "Airport - Bullfrog");
    CHECK(routes[0].route_type == RouteType::Bus);
    CHECK(routes[0].route_text_color.empty());
    CHECK(routes[0].route_color.empty());
    CHECK(routes[0].route_desc.empty());

    const auto & route = feed.get_route("AB");
    CHECK(route);
}

TEST_CASE("Trips")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_trips() == access::result_code::OK);

    const auto & trips = feed.get_trips();
    REQUIRE(trips.size() == 11);

    CHECK(trips[0].block_id == "1");
    CHECK(trips[0].route_id == "AB");
    CHECK(trips[0].direction_id == trip_direction_id::DefaultDirection);
    CHECK(trips[0].trip_headsign == "to Bullfrog");
    CHECK(trips[0].shape_id.empty());
    CHECK(trips[0].service_id == "FULLW");
    CHECK(trips[0].trip_id == "AB1");

    const auto & trip = feed.get_trip("AB1");
    REQUIRE(trip);
    CHECK(trip.value().trip_short_name.empty());
}

TEST_CASE("Stops")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_stops() == access::result_code::OK);

    const auto & stops = feed.get_stops();
    REQUIRE(stops.size() == 9);
    CHECK(stops[0].stop_lat == 36.425288);
    CHECK(stops[0].stop_lon == -117.133162);
    CHECK(stops[0].stop_code.empty());
    CHECK(stops[0].stop_url.empty());
    CHECK(stops[0].stop_id == "FUR_CREEK_RES");
    CHECK(stops[0].stop_desc.empty());
    CHECK(stops[0].stop_name == "Furnace Creek Resort (Demo)");
    CHECK(stops[0].location_type == stop_location_type::GenericNode);
    CHECK(stops[0].zone_id.empty());

    auto const & stop = feed.get_stop("FUR_CREEK_RES");
    REQUIRE(stop);
}

TEST_CASE("StopTimes")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_stop_times() == access::result_code::OK);

    const auto & stop_times = feed.get_stop_times();
    REQUIRE(stop_times.size() == 28);

    CHECK(stop_times[0].trip_id == "STBA");
    CHECK(stop_times[0].arrival_time == time(06, 00, 00));
    CHECK(stop_times[0].departure_time == time(06, 00, 00));
    CHECK(stop_times[0].stop_id == "STAGECOACH");
    CHECK(stop_times[0].stop_sequence == 1);
    CHECK(stop_times[0].stop_headsign.empty());
    CHECK(stop_times[0].pickup_type == stop_time_boarding::RegularlyScheduled);
    CHECK(stop_times[0].drop_off_type == stop_time_boarding::RegularlyScheduled);

    CHECK_EQ(feed.get_stop_times_for_stop("STAGECOACH").size(), 3);
    CHECK_EQ(feed.get_stop_times_for_trip("STBA").size(), 2);
}

TEST_CASE("Shapes")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_shapes() == access::result_code::OK);

    const auto & shapes = feed.get_shapes();
    REQUIRE(shapes.size() == 8);
    CHECK(shapes[0].shape_id == "10237");
    CHECK(shapes[0].shape_pt_lat == 43.5176524709);
    CHECK(shapes[0].shape_pt_lon == -79.6906570431);
    CHECK(shapes[0].shape_pt_sequence == 50017);
    CHECK(shapes[0].shape_dist_traveled == 12669);

    const auto & shape = feed.get_shape("10237");
    CHECK(shape.size() == 4);
}

TEST_CASE("Calendar")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_calendar() == access::result_code::OK);

    const auto & calendar = feed.get_calendar();
    REQUIRE(calendar.size() == 2);
    CHECK(calendar[0].service_id == "FULLW");
    CHECK(calendar[0].start_date == date(2007, 01, 01));
    CHECK(calendar[0].end_date == date(2010, 12, 31));
    CHECK(calendar[0].monday == calendar_availability::Available);
    CHECK(calendar[0].sunday == calendar_availability::Available);

    const auto & calendar_for_service = feed.get_calendar("FULLW");
    CHECK(calendar_for_service);
}

TEST_CASE("Calendar dates")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_calendar_dates() == access::result_code::OK);

    const auto & calendar_dates = feed.get_calendar_dates();
    REQUIRE(calendar_dates.size() == 1);
    CHECK(calendar_dates[0].service_id == "FULLW");
    CHECK(calendar_dates[0].date == date(2007, 06, 04));
    CHECK(calendar_dates[0].exception_type == calendar_date_exception::Removed);

    const auto & calendar_dates_for_service = feed.get_calendar_dates("FULLW");
    CHECK(calendar_dates_for_service.size() == 1);
}

TEST_CASE("Frequencies")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_frequencies() == access::result_code::OK);

    const auto & frequencies = feed.get_frequencies();
    REQUIRE(frequencies.size() == 11);
    CHECK(frequencies[0].trip_id == "STBA");
    CHECK(frequencies[0].start_time == time(6, 00, 00));
    CHECK(frequencies[0].end_time == time(22, 00, 00));
    CHECK(frequencies[0].headway_secs == 1800);

    const auto & frequencies_for_trip = feed.get_frequencies("CITY1");
    CHECK(frequencies_for_trip.size() == 5);
}

TEST_CASE("Fare attributes")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_fare_attributes() == access::result_code::OK);

    const auto & attributes = feed.get_fare_attributes();
    REQUIRE(attributes.size() == 2);
    CHECK(attributes[0].fare_id == "p");
    CHECK(attributes[0].price == 1.25);
    CHECK(attributes[0].currency_type == "USD");
    CHECK(attributes[0].payment_method == FarePayment::OnBoard);
    CHECK(attributes[0].transfers == FareTransfers::No);
    CHECK(attributes[0].transfer_duration == 0);

    const auto & attributes_for_id = feed.get_fare_attributes("a");
    REQUIRE(attributes_for_id.size() == 1);
    CHECK(attributes_for_id[0].price == 5.25);
}

TEST_CASE("Fare rules")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_fare_rules() == access::result_code::OK);

    const auto & fare_rules = feed.get_fare_rules();
    REQUIRE(fare_rules.size() == 4);
    CHECK(fare_rules[0].fare_id == "p");
    CHECK(fare_rules[0].route_id == "AB");

    const auto & rules_for_id = feed.get_fare_rules("p");
    REQUIRE(rules_for_id.size() == 3);
    CHECK(rules_for_id[1].route_id == "STBA");
}

TEST_CASE("Levels")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_levels() == access::result_code::OK);

    const auto & levels = feed.get_levels();
    REQUIRE(levels.size() == 3);
    CHECK(levels[0].level_id == "U321L1");
    CHECK(levels[0].level_index == -1.5);

    const auto & level = feed.get_level("U321L2");
    REQUIRE(level);

    CHECK(level.value().level_index == -2);
    CHECK(level.value().level_name == "Vestibul2");
}

TEST_CASE("Pathways")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_pathways() == access::result_code::OK);

    const auto & pathways = feed.get_pathways();
    REQUIRE(pathways.size() == 3);
    CHECK(pathways[0].pathway_id == "T-A01C01");
    CHECK(pathways[0].from_stop_id == "1073S");
    CHECK(pathways[0].to_stop_id == "1098E");
    CHECK(pathways[0].pathway_mode == PathwayMode::Stairs);
    CHECK(pathways[0].signposted_as == "Sign1");
    CHECK(pathways[0].reversed_signposted_as == "Sign2");
    CHECK(pathways[0].is_bidirectional == PathwayDirection::Bidirectional);

    const auto & pathways_by_id = feed.get_pathways("T-A01D01");
    REQUIRE(pathways_by_id.size() == 2);
    CHECK(pathways_by_id[0].is_bidirectional == PathwayDirection::Unidirectional);
    CHECK(pathways_by_id[0].reversed_signposted_as.empty());
}

TEST_CASE("Translations")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_translations() == access::result_code::OK);

    const auto & translations = feed.get_translations();
    REQUIRE(translations.size() == 1);
    CHECK(translations[0].table_name == "stop_times");
    CHECK(translations[0].field_name == "stop_headsign");
    CHECK(translations[0].language == "en");
    CHECK(translations[0].translation == "Downtown");
    CHECK(translations[0].record_id.empty());
    CHECK(translations[0].record_sub_id.empty());
    CHECK(translations[0].field_value.empty());

    CHECK_EQ(feed.get_translations("stop_times").size(), 1);
}

TEST_CASE("Attributions")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_attributions() == access::result_code::OK);

    const auto & attributions = feed.get_attributions();
    REQUIRE(attributions.size() == 1);
    CHECK(attributions[0].attribution_id == "0");
    CHECK(attributions[0].organization_name == "Test inc");
    CHECK(attributions[0].is_producer == AttributionRole::Yes);
    CHECK(attributions[0].is_operator == AttributionRole::No);
    CHECK(attributions[0].is_authority == AttributionRole::No);
    CHECK(attributions[0].attribution_url == "https://test.pl/gtfs/");
    CHECK(attributions[0].attribution_email.empty());
    CHECK(attributions[0].attribution_phone.empty());
}

TEST_CASE("feed info")
{
    feed feed(TEST_FOLDER_PATH"/resources/sample_feed");
    REQUIRE(feed.read_feed_info() == access::result_code::OK);

    const auto & info = feed.get_feed_info();

    CHECK(info.feed_publisher_name == "Test Solutions, Inc.");
    CHECK(info.feed_publisher_url == "http://test");
    CHECK(info.feed_lang == "en");
}
#endif

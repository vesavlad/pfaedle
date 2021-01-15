#include "catch_amalgamated.hpp"
#include <gtfs/exceptions.h>
#include <gtfs/time.h>
#include "config.h"

namespace pafedle::gtfs
{

TEST_CASE("Time in H:MM:SS format")
{
    pfaedle::gtfs::time stop_time("0:19:00");
    REQUIRE(stop_time.is_provided());

    REQUIRE(stop_time.get_hh_mm_ss() == std::make_tuple(0, 19, 0));
    REQUIRE(stop_time.get_raw_time() == "0:19:00");
    REQUIRE(stop_time.get_total_seconds() == 19 * 60);
}

TEST_CASE("Time in HH:MM:SS format")
{
    pfaedle::gtfs::time stop_time("39:45:30");
    REQUIRE(stop_time.get_hh_mm_ss() == std::make_tuple(39, 45, 30));
    REQUIRE(stop_time.get_raw_time() == "39:45:30");
    REQUIRE(stop_time.get_total_seconds() == 39 * 60 * 60 + 45 * 60 + 30);
}

TEST_CASE("Time from integers 1")
{
    pfaedle::gtfs::time stop_time(14, 30, 0);
    REQUIRE(stop_time.get_hh_mm_ss() == std::make_tuple(14, 30, 0));
    REQUIRE(stop_time.get_raw_time() == "14:30:00");
    REQUIRE(stop_time.get_total_seconds() == 14 * 60 * 60 + 30 * 60);
}

TEST_CASE("Time from integers 2")
{
    pfaedle::gtfs::time stop_time(3, 0, 0);
    REQUIRE(stop_time.get_hh_mm_ss() == std::make_tuple(3, 0, 0));
    REQUIRE(stop_time.get_raw_time() == "03:00:00");
    REQUIRE(stop_time.get_total_seconds() == 3 * 60 * 60);
}

TEST_CASE("Invalid time format")
{
    CHECK_THROWS_AS(pfaedle::gtfs::time("12/10/00"), pfaedle::gtfs::invalid_field_format);
    CHECK_THROWS_AS(pfaedle::gtfs::time("12:100:00"), pfaedle::gtfs::invalid_field_format);
    CHECK_THROWS_AS(pfaedle::gtfs::time("12:10:100"), pfaedle::gtfs::invalid_field_format);
}

TEST_CASE("Time not provided")
{
    pfaedle::gtfs::time stop_time("");
    CHECK(!stop_time.is_provided());
}
}

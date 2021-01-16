#include <gtfs/date.h>
#include <gtfs/exceptions/invalid_field_format.h>
#include <tuple>

#include "catch_amalgamated.hpp"

using namespace pfaedle::gtfs;

TEST_CASE("Date not provided")
{
    date date("");
    CHECK(!date.is_provided());
}

TEST_CASE("Invalid date format")
{
    // Violation of the format YYYYMMDD:
    CHECK_THROWS_AS(date("1999314"), invalid_field_format);
    CHECK_THROWS_AS(date("20081414"), invalid_field_format);
    CHECK_THROWS_AS(date("20170432"), invalid_field_format);

    // Count of days in february (leap year):
    CHECK_THROWS_AS(date("20200230"), invalid_field_format);
    // Count of days in february (not leap year):
    CHECK_THROWS_AS(date("20210229"), invalid_field_format);

    // Count of days in months with 30 days:
    CHECK_THROWS_AS(date("19980431"), invalid_field_format);
    CHECK_THROWS_AS(date("19980631"), invalid_field_format);
    CHECK_THROWS_AS(date("19980931"), invalid_field_format);
    CHECK_THROWS_AS(date("19981131"), invalid_field_format);
}

TEST_CASE("Date from string 1")
{
    date date("20230903");
    CHECK(date.get_yyyy_mm_dd() == std::make_tuple(2023, 9, 3));
    CHECK(date.get_raw_date() == "20230903");
    CHECK(date.is_provided());
}

TEST_CASE("Date from string 2")
{
    date date("20161231");
    CHECK(date.get_yyyy_mm_dd() == std::make_tuple(2016, 12, 31));
    CHECK(date.get_raw_date() == "20161231");
    CHECK(date.is_provided());
}

TEST_CASE("Date from string 3")
{
    date date("20200229");
    CHECK(date.get_yyyy_mm_dd() == std::make_tuple(2020, 2, 29));
    CHECK(date.get_raw_date() == "20200229");
    CHECK(date.is_provided());
}

TEST_CASE("Date from integers")
{
    date date(2022, 8, 16);
    CHECK(date.get_yyyy_mm_dd() == std::make_tuple(2022, 8, 16));

    CHECK(date.get_raw_date() == "20220816");
    CHECK(date.is_provided());
}

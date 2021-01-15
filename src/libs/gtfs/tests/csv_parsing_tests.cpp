#include "catch_amalgamated.hpp"
#include "../src/gtfs/access/csv_parser.h"

using namespace pfaedle::gtfs::access;

TEST_CASE("Record with empty values")
{
    const auto res = csv_parser::split_record(",, ,");
    REQUIRE(res.size() == 4);
    for (const auto & token : res)
        CHECK(token.empty());
}

TEST_CASE("Header with UTF BOM")
{
    const auto res = csv_parser::split_record("\xef\xbb\xbfroute_id, agency_id", true);
    REQUIRE(res.size() == 2);
    CHECK(res[0] == "route_id");
    CHECK(res[1] == "agency_id");
}

TEST_CASE("Quotation marks")
{
    const auto res = csv_parser::split_record(R"(27681 ,,"Sisters, OR",,"44.29124",1)");
    REQUIRE(res.size() == 6);
    CHECK(res[2] == "Sisters, OR");
    CHECK(res[4] == "44.29124");
    CHECK(res[5] == "1");
}

TEST_CASE("Not wrapped quotation marks")
{
    const auto res = csv_parser::split_record(R"(Contains "quotes", commas and text)");
    REQUIRE(res.size() == 2);
    CHECK(res[0] == R"(Contains "quotes")");
    CHECK(res[1] == "commas and text");
}

TEST_CASE("Wrapped quotation marks")
{
    const auto res = csv_parser::split_record(R"("Contains ""quotes"", commas and text")");
    REQUIRE(res.size() == 1);
    CHECK(res[0] == R"(Contains "quotes", commas and text)");
}

TEST_CASE("Double wrapped quotation marks")
{
    const auto res = csv_parser::split_record(R"(""Double quoted text"")");
    REQUIRE(res.size() == 1);
}

TEST_CASE("Read quoted empty values")
{
    const auto res = csv_parser::split_record(",\"\"");
    REQUIRE(res.size() == 2);
    CHECK(res[0] == "");
    CHECK(res[1] == "");
}
TEST_CASE("Read quoted quote")
{
    const auto res = csv_parser::split_record(",\"\"\"\"");
    REQUIRE(res.size() == 2);
    CHECK(res[0] == "");
    CHECK(res[1] == "\"");
}

TEST_CASE("Read quoted double quote")
{
    const auto res = csv_parser::split_record(",\"\"\"\"\"\"");
    REQUIRE(res.size() == 2);
    CHECK(res[0] == "");
    CHECK(res[1] == "\"\"");
}

TEST_CASE("Read quoted values with quotes in begin")
{
    const auto res = csv_parser::split_record(",\"\"\"Name\"\" and some other\"");
    REQUIRE(res.size() == 2);
    CHECK(res[0] == "");
    CHECK(res[1] == "\"Name\" and some other");
}

TEST_CASE("Read quoted values with quotes at end")
{
    const auto res = csv_parser::split_record(",\"Text and \"\"Name\"\"\"");
    REQUIRE(res.size() == 2);
    CHECK(res[0] == "");
    CHECK(res[1] == "Text and \"Name\"");
}

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch_amalgamated.hpp"
#include <gtfs/misc.h>
#include <gtfs/exceptions.h>
#include <gtfs/time.h>
#include "config.h"

namespace pafedle::gtfs
{
TEST_CASE( "Quoting works", "[quote]" ) {
    REQUIRE( pfaedle::gtfs::quote_text("what the fuck") == "\"what the fuck\"" );
    REQUIRE( pfaedle::gtfs::quote_text("this is the 2`nd try") == "\"this is the 2`nd try\"" );
    REQUIRE( pfaedle::gtfs::quote_text("this is the 3'rd try") == "\"this is the 3'rd try\"" );
    REQUIRE( pfaedle::gtfs::quote_text("this is \" the 4'th try") == "\"this is \"\" the 4'th try\"" );
}

TEST_CASE( "Unquoting works", "[quote]" ) {
    REQUIRE( pfaedle::gtfs::unquote_text("\"what the fuck\"") == "what the fuck" );
    REQUIRE( pfaedle::gtfs::unquote_text("\"this is the 2`nd try\"") == "this is the 2`nd try" );
    REQUIRE( pfaedle::gtfs::unquote_text("\"this is the 3'rd try\"") == "this is the 3'rd try" );
    REQUIRE( pfaedle::gtfs::unquote_text("\"this is \"\" the 4'th try\"") == "this is \" the 4'th try" );
}

TEST_CASE( "vectors can be sized and resized", "[vector]" ) {

    std::vector<int> v( 5 );

    REQUIRE( v.size() == 5 );
    REQUIRE( v.capacity() >= 5 );

    SECTION( "resizing bigger changes size and capacity" ) {
        v.resize( 10 );

        REQUIRE( v.size() == 10 );
        REQUIRE( v.capacity() >= 10 );
    }
    SECTION( "resizing smaller changes size but not capacity" ) {
        v.resize( 0 );

        REQUIRE( v.size() == 0 );
        REQUIRE( v.capacity() >= 5 );
    }
    SECTION( "reserving bigger changes capacity but not size" ) {
        v.reserve( 10 );

        REQUIRE( v.size() == 5 );
        REQUIRE( v.capacity() >= 10 );
    }
    SECTION( "reserving smaller does not change size or capacity" ) {
        v.reserve( 0 );

        REQUIRE( v.size() == 5 );
        REQUIRE( v.capacity() >= 5 );
    }
}

SCENARIO( "vectors can be sized and resized", "[vector]" ) {

    GIVEN( "A vector with some items" ) {
        std::vector<int> v( 5 );

        REQUIRE( v.size() == 5 );
        REQUIRE( v.capacity() >= 5 );

        WHEN( "the size is increased" ) {
            v.resize( 10 );

            THEN( "the size and capacity change" ) {
                REQUIRE( v.size() == 10 );
                REQUIRE( v.capacity() >= 10 );
            }
        }
        WHEN( "the size is reduced" ) {
            v.resize( 0 );

            THEN( "the size changes but not capacity" ) {
                REQUIRE( v.size() == 0 );
                REQUIRE( v.capacity() >= 5 );
            }
        }
        WHEN( "more capacity is reserved" ) {
            v.reserve( 10 );

            THEN( "the capacity changes but not the size" ) {
                REQUIRE( v.size() == 5 );
                REQUIRE( v.capacity() >= 10 );
            }
        }
        WHEN( "less capacity is reserved" ) {
            v.reserve( 0 );

            THEN( "neither size nor capacity are changed" ) {
                REQUIRE( v.size() == 5 );
                REQUIRE( v.capacity() >= 5 );
            }
        }
    }
}
}

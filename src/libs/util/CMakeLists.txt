file(GLOB_RECURSE util_SRC src/*.cpp)

add_library(util ${util_SRC})
target_include_directories(util PUBLIC include)

find_package( ZLIB )
if (ZLIB_FOUND)
    include_directories( ${ZLIB_INCLUDE_DIRS} )
    target_link_libraries( util ${ZLIB_LIBRARIES} )
    add_definitions( -DZLIB_FOUND=${ZLIB_FOUND} )
endif( ZLIB_FOUND )

#add_subdirectory(tests)

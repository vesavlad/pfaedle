file(GLOB_RECURSE stei-gtfs-src *.cpp)
add_library(stei-gtfs ${stei-gtfs-src})
target_include_directories(stei-gtfs PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

set_target_properties(stei-gtfs PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF)

add_subdirectory(tests)

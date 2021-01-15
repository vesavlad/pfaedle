set(test_folder_path ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "CMAKE_CURRENT_BINARY_DIR=" ${CMAKE_CURRENT_BINARY_DIR})
configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
add_executable(stei-gtfs-tests
        main.cpp
        time_tests.cpp
        date_tests.cpp
        feed_reading.cpp
        csv_parsing_tests.cpp
        catch_amalgamated.cpp)

target_link_libraries(stei-gtfs-tests PUBLIC stei-gtfs)
target_include_directories(stei-gtfs-tests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../src/gtfs/access/")
target_compile_features(stei-gtfs-tests PRIVATE cxx_std_17)
add_test("stei-gtfs-tests" "stei-gtfs-tests" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} --verbose)

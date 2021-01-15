set(test_folder_path ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
add_executable(stei-gtfs-tests
        main.cpp
        catch_amalgamated.cpp)

target_link_libraries(stei-gtfs-tests
        PUBLIC stei-gtfs)

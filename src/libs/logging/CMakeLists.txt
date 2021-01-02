cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

add_library(logging STATIC
        src/logger.cpp
        src/logging.cpp
        src/scoped_timer.cpp
        src/manual_timer.cpp)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(logging PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(logging
        PRIVATE spdlog)

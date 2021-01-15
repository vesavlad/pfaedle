add_library(pfaedle-main-lib app.cpp)
target_link_libraries(pfaedle-main-lib
        PUBLIC logging
        PUBLIC stei-gtfs
        PUBLIC pfaedle-lib
        PRIVATE pfaedle-generated
        PRIVATE pfaedle-util
        PRIVATE configparser
        -lpthread)

add_executable(pfaedle main.cpp)
target_link_libraries(pfaedle
        pfaedle-main-lib
        exceptions)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build")
set_target_properties(pfaedle
        PROPERTIES
        EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build"
        ENABLE_EXPORTS ON)

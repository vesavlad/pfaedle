add_executable(pfaedle
        main.cpp
        app.cpp)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build")
set_target_properties(pfaedle PROPERTIES EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build")
target_link_libraries(pfaedle
        exceptions
        logging
        pfaedle-generated
        pfaedle-lib
        pfaedle-util
        configparser
        cppgtfs
        -lpthread)


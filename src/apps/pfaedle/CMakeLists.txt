set(pfaedle_main main.cpp)
add_executable(pfaedle main.cpp)

target_link_libraries(pfaedle
        logging
        pfaedle-generated
        pfaedle-lib
        pfaedle-util
        configparser
        cppgtfs
        -lpthread)

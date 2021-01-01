set(pfaedle_main main.cpp)
add_executable(pfaedle main.cpp)
target_link_libraries(pfaedle
        PRIVATE pfaedle-generated
        pfaedle_dep
        util
        configparser
        cppgtfs
        -lpthread)

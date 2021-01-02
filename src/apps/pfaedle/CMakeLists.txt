add_executable(pfaedle
        main.cpp
        app.cpp)

target_link_libraries(pfaedle
        logging
        pfaedle-generated
        pfaedle-lib
        pfaedle-util
        configparser
        cppgtfs
        -lpthread)

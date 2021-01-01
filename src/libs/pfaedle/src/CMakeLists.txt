file(GLOB_RECURSE pfaedle_SRC *.cpp)

set(pfaedle_main PfaedleMain.cpp)

list(REMOVE_ITEM pfaedle_SRC ${pfaedle_main})

include_directories(${PFAEDLE_INCLUDE_DIR})

add_executable(pfaedle ${pfaedle_main})
add_library(pfaedle_dep ${pfaedle_SRC})
target_link_libraries(pfaedle_dep
        PRIVATE pfaedle-generated
        PUBLIC util
        PUBLIC configparser
        PUBLIC cppgtfs
        PUBLIC pfxml)

include_directories(pfaedle_dep PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../)


target_link_libraries(pfaedle
        PRIVATE pfaedle-generated
        pfaedle_dep
        util
        configparser
        cppgtfs
        -lpthread)

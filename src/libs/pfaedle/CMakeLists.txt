################################
# PFAEDLE Version
################################
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE pfaedle_git_branch
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND git --no-pager log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE pfaedle_git_ref
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND git describe --always --tags --dirty=-dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE pfaedle_git_tag
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file (
        "src/config.h.in"
        "${CMAKE_BINARY_DIR}/generated/pfaedle/config.h"
)

file(GLOB_RECURSE pfaedle_SRC *.cpp)
add_library(pfaedle-lib ${pfaedle_SRC})

target_link_libraries(pfaedle-lib
        PUBLIC logging
        PRIVATE osmium
        PRIVATE expat
        PRIVATE protozero
        PUBLIC stei-gtfs
        PUBLIC pfaedle-generated
        PUBLIC pfaedle-util
        PUBLIC configparser
        PUBLIC pugixml
        PUBLIC z
        )
target_include_directories(pfaedle-lib PUBLIC include ${osmpbf_INCLUDE_DIRS})


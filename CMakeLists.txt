cmake_minimum_required(VERSION 3.15)

project (pfaedle C CXX)
include(cmake/buildcache.cmake)
include(cmake/pkg.cmake)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build")


find_package(OpenMP)
if (OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()


# Quickfix for GCC 7
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wno-implicit-fallthrough")
endif()
# set compiler flags, see http://stackoverflow.com/questions/7724569/debug-vs-release-in-cmake
if(OPENMP_FOUND)
	set(CMAKE_CXX_FLAGS            "-fopenmp -Ofast -fno-signed-zeros -fno-trapping-math -Wall -Wno-format-extra-args -Wextra -Wformat-nonliteral -Wformat-security -Wformat=2 -Wextra -Wno-implicit-fallthrough -pedantic")
else()
	message(WARNING "Configuring without OpenMP!")
	set(CMAKE_CXX_FLAGS            "-Ofast -fno-signed-zeros -fno-trapping-math -Wall -Wno-format-extra-args -Wextra -Wformat-nonliteral -Wformat-security -Wformat=2 -Wextra -Wno-implicit-fallthrough -pedantic")
endif()
set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g -DPFAEDLE_DBG=1")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} -g")

# export compile commands to tools like clang
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

add_library(pfaedle-generated INTERFACE)
target_include_directories(pfaedle-generated INTERFACE ${CMAKE_BINARY_DIR}/generated)


################################
# Linter
################################
option(PFAEDLE_LINT "Run clang-tidy with the compiler." OFF)
if(PFAEDLE_LINT)
	# clang-tidy will be run on all targets defined hereafter
	include(cmake/clang-tidy.cmake)
endif()

add_subdirectory(src)

# tests

add_test("utilTest" utilTest)

# custom eval target

add_custom_target(
	eval
	COMMAND make
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}//eval
)

# handles install target
install(
	FILES pfaedle.cfg DESTINATION etc/${PROJECT_NAME} COMPONENT config PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
)

install(
	FILES build/pfaedle DESTINATION bin
	PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE COMPONENT binaries
)

################################
# clang-format check
################################
find_program(CLANG_FORMAT_COMMAND NAMES clang-format clang-format-9)
add_custom_target(format-check
        COMMAND find
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pfaedle
        ${CMAKE_CURRENT_SOURCE_DIR}/src/util
        -type f
(
        -name "*.cpp"
        -o
        -name "*.h"
        )
        -print0
        | xargs -0 ${CLANG_FORMAT_COMMAND} -i
        COMMAND git status --porcelain
        COMMAND git status --porcelain | xargs -I {} -0 test -z \"{}\"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking source code formatting"
        VERBATIM
        )

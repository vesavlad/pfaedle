cmake_minimum_required(VERSION 3.17)

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


# set compiler flags, see http://stackoverflow.com/questions/7724569/debug-vs-release-in-cmake
if(OPENMP_FOUND)
	set(CMAKE_CXX_FLAGS            "-fopenmp -Ofast -fno-signed-zeros -fno-trapping-math -Wall -Wno-format-extra-args -Wextra -Wformat-nonliteral -Wformat-security -Wformat=2 -Wextra -Wno-implicit-fallthrough -pedantic")
else()
	message(WARNING "Configuring without OpenMP!")
	set(CMAKE_CXX_FLAGS            "-Ofast -fno-signed-zeros -fno-trapping-math -Wall -Wno-format-extra-args -Wextra -Wformat-nonliteral -Wformat-security -Wformat=2 -Wextra -Wno-implicit-fallthrough -pedantic")
endif()
set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g -DLOGLEVEL=3 -DPFAEDLE_DBG=1")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS} -DLOGLEVEL=2")
set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS} -DLOGLEVEL=2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} -g -DLOGLEVEL=3")

# export compile commands to tools like clang
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler-specific C++17 activation.
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	execute_process(
		COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
	if ((GCC_VERSION VERSION_GREATER 7.5 OR GCC_VERSION VERSION_EQUAL 7.5))
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
	else ()
		message(FATAL_ERROR "${PROJECT_NAME} requires g++ 7.5 or greater!")
	endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++17")
else ()
	message(FATAL_ERROR "Your C++ compiler does not support C++17.")
endif ()

add_library(pfaedle-generated INTERFACE)
target_include_directories(pfaedle-generated INTERFACE ${CMAKE_BINARY_DIR}/generated)

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
        "src/pfaedle/config.h.in"
        "${CMAKE_BINARY_DIR}/generated/pfaedle/config.h"
)


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

add_executable(util_test TestMain.cpp)
target_link_libraries(util_test pfaedle-util)
# tests

add_test("util_test" util_test)

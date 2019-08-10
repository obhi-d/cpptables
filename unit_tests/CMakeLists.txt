
project(cpphash_general_tests)

## Basic tests
add_executable(test_cpphash_general general_test.cpp)
add_test(cpphash_general test_cpphash_general)

target_link_libraries(test_cpphash_general cpphash)



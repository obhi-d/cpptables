
project(cpptables_general_tests)

## Basic tests
add_executable(test_cpptables_general general_test.cpp)
add_test(cpptables_general test_cpptables_general)

target_link_libraries(test_cpptables_general cpptables)




project(cpptables_tests)

## Basic tests
add_executable(cpptables_tests_validation validation.cpp)
add_test(validation cpptables_tests_validation)
target_compile_features(cpptables_tests_validation PUBLIC cxx_std_17)

target_link_libraries(cpptables_tests_validation cpptables)




project(cpptables_general_tests)

include(ExternalProject)

ExternalProject_Add(Catch2
#  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/Catch2"
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/Catch2
  STEP_TARGETS install 
  CMAKE_ARGS 
  	-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/out/external/install
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}
		-DCATCH_BUILD_TESTING=OFF
)

## Basic tests
macro(validity_test test_name definitions compile_flags link_options)
  add_executable(cpptables-unit-test-validity-${test_name} 
    validity/main.cpp
    validity/validation.cpp
    )
  target_link_libraries(cpptables-unit-test-validity-${test_name} cpptables)
  add_test(validity-${test_name} cpptables-unit-test-validity-${test_name})
  add_dependencies(cpptables-unit-test-validity-${test_name} Catch2-install)
  target_include_directories(cpptables-unit-test-validity-${test_name} PRIVATE "${CMAKE_SOURCE_DIR}/out/external/install/include")
  target_include_directories(cpptables-unit-test-validity-${test_name} PRIVATE "${CMAKE_SOURCE_DIR}/include")
  target_compile_definitions(cpptables-unit-test-validity-${test_name} PRIVATE ${definitions})
  target_compile_options(cpptables-unit-test-validity-${test_name} PRIVATE ${compile_flags})
  target_compile_features(cpptables-unit-test-validity-${test_name} PRIVATE cxx_std_17)
  target_link_options(cpptables-unit-test-validity-${test_name} PRIVATE ${link_options})
endmacro()

set(CPPTABLES_COMMON_CXX_FLAGS "")
set(CPPTABLES_COMMON_CXX_LINK_FLAGS "")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CPPTABLES_COMMON_CXX_FLAGS "-fsanitize=address;-fno-omit-frame-pointer;-ftest-coverage;-fprofile-instr-generate;-fcoverage-mapping")
  set(CPPTABLES_COMMON_CXX_LINK_FLAGS "-fsanitize=address;-fprofile-instr-generate")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CPPTABLES_COMMON_CXX_FLAGS "-fsanitize=address;-fno-omit-frame-pointer")
  set(CPPTABLES_COMMON_CXX_LINK_FLAGS "-fsanitize=address")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(CPPTABLES_COMMON_CXX_FLAGS "")
  set(CPPTABLES_COMMON_CXX_LINK_FLAGS "")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CPPTABLES_COMMON_CXX_FLAGS "")
  set(CPPTABLES_COMMON_CXX_LINK_FLAGS "")
endif()

validity_test("cpp" "" "${CPPTABLES_COMMON_CXX_FLAGS}" "${CPPTABLES_COMMON_CXX_LINK_FLAGS}")

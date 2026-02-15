cmake_minimum_required(VERSION 3.20)

# Find Catch2
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, tests will not be built")
    return()
endif()

# Test sources
set(TEST_SOURCES
    test_main.cpp
    RouterTests.cpp
    MiddlewareTests.cpp
)

# Create test executable
add_executable(http-framework-tests ${TEST_SOURCES})

target_link_libraries(http-framework-tests
    PRIVATE
        http-framework
        Catch2::Catch2WithMain
)

# Discover tests
include(CTest)
include(Catch)
catch_discover_tests(http-framework-tests)

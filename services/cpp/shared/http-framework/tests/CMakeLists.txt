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
    ServiceProviderTests.cpp
    ServiceScopeTests.cpp
    PluginManagerTests.cpp
    NamespaceTests.cpp
    RouteValidationTests.cpp
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

# Build test plugin (shared library that tests will load dynamically)
add_library(test-plugin SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/../plugins/TestPlugin.cpp
)

target_include_directories(test-plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Ensure test plugin is built in the same directory as tests
set_target_properties(test-plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    PREFIX "lib"
    SUFFIX ".so"
)

# Make test plugin depend on being built before tests
add_dependencies(http-framework-tests test-plugin)

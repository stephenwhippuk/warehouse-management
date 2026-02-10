# Find Catch2
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, skipping tests")
    return()
endif()

# Test sources
set(TEST_SOURCES
    HttpIntegrationTests.cpp
)

# Test executable
add_executable(order-service-tests ${TEST_SOURCES})

# Include directories
target_include_directories(order-service-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link libraries
target_link_libraries(order-service-tests
    Poco::Net
    Poco::NetSSL
    Poco::Util
    Poco::Foundation
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    Catch2::Catch2WithMain
)

# Discover tests
include(CTest)
include(Catch)
catch_discover_tests(order-service-tests)

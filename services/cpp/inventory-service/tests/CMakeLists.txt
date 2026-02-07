# Test Configuration
cmake_minimum_required(VERSION 3.20)

# Find Catch2
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    # Fallback to FetchContent if Catch2 is not installed
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Test executable
add_executable(inventory-service-tests
    test_main.cpp
    InventoryTests.cpp
)

# Link libraries
target_link_libraries(inventory-service-tests
    PRIVATE
    Catch2::Catch2
    ${Boost_LIBRARIES}
    Poco::Net
    Poco::Util
    Poco::Foundation
    ${PostgreSQL_LIBRARIES}
    pqxx
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Include directories
target_include_directories(inventory-service-tests
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Add test sources (without main.cpp)
target_sources(inventory-service-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/models/Inventory.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/Logger.cpp
)

# Discover tests
include(CTest)
include(Catch)
catch_discover_tests(inventory-service-tests)

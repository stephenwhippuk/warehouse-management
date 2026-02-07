cmake_minimum_required(VERSION 3.20)

# Find testing framework
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    find_package(GTest QUIET)
endif()

if(Catch2_FOUND)
    message(STATUS "Using Catch2 for testing")
    
    # Test sources
    set(TEST_SOURCES
        test_main.cpp
        WarehouseTests.cpp
        LocationTests.cpp
    )
    
    add_executable(warehouse-service-tests ${TEST_SOURCES})
    
    target_link_libraries(warehouse-service-tests
        PRIVATE
            Catch2::Catch2WithMain
            nlohmann_json::nlohmann_json
            spdlog::spdlog
    )
    
    target_include_directories(warehouse-service-tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
    )
    
    include(CTest)
    include(Catch)
    catch_discover_tests(warehouse-service-tests)
    
elseif(GTest_FOUND)
    message(STATUS "Using Google Test for testing")
    
    # Test sources
    set(TEST_SOURCES
        test_main_gtest.cpp
        WarehouseTests.cpp
        LocationTests.cpp
    )
    
    add_executable(warehouse-service-tests ${TEST_SOURCES})
    
    target_link_libraries(warehouse-service-tests
        PRIVATE
            GTest::GTest
            GTest::Main
            nlohmann_json::nlohmann_json
            spdlog::spdlog
    )
    
    target_include_directories(warehouse-service-tests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
    )
    
    include(GoogleTest)
    gtest_discover_tests(warehouse-service-tests)
    
else()
    message(WARNING "No testing framework found. Tests will not be built.")
endif()

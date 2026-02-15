cmake_minimum_required(VERSION 3.20)

# Find Catch2 (if available)
find_package(Catch2 3 QUIET)

if(Catch2_FOUND)
    # Event tests
    add_executable(event_tests
        EventTests.cpp
    )
    target_link_libraries(event_tests
        PRIVATE
            warehouse-messaging
            Catch2::Catch2WithMain
    )
    
    # Publisher tests
    add_executable(publisher_tests
        PublisherTests.cpp
    )
    target_link_libraries(publisher_tests
        PRIVATE
            warehouse-messaging
            Catch2::Catch2WithMain
    )
    
    # Consumer tests
    add_executable(consumer_tests
        ConsumerTests.cpp
    )
    target_link_libraries(consumer_tests
        PRIVATE
            warehouse-messaging
            Catch2::Catch2WithMain
    )
    
    # Register tests with CTest
    include(CTest)
    include(Catch)
    catch_discover_tests(event_tests)
    catch_discover_tests(publisher_tests)
    catch_discover_tests(consumer_tests)
    
    message(STATUS "Tests enabled (Catch2 found)")
else()
    message(STATUS "Tests disabled (Catch2 not found)")
endif()

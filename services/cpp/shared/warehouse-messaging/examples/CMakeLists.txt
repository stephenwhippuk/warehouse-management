cmake_minimum_required(VERSION 3.20)

# Simple Publisher Example
add_executable(simple_publisher simple_publisher.cpp)
target_link_libraries(simple_publisher 
    PRIVATE 
        warehouse-messaging
)

# Simple Consumer Example
add_executable(simple_consumer simple_consumer.cpp)
target_link_libraries(simple_consumer 
    PRIVATE 
        warehouse-messaging
)

# Install examples
install(TARGETS simple_publisher simple_consumer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)

message(STATUS "Building examples:")
message(STATUS "  - simple_publisher")
message(STATUS "  - simple_consumer")

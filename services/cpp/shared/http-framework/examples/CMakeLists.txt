cmake_minimum_required(VERSION 3.20)

# Basic server example
add_executable(basic-server basic_server.cpp)
target_link_libraries(basic-server http-framework)

# Install example
install(TARGETS basic-server
    RUNTIME DESTINATION bin/examples
)

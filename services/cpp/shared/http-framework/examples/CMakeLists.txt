cmake_minimum_required(VERSION 3.20)

# Basic server example (original, without DI)
add_executable(basic-server basic_server.cpp)
target_link_libraries(basic-server http-framework)

# DI-enabled server example (demonstrates dependency injection)
add_executable(di-server di_server.cpp)
target_link_libraries(di-server http-framework)

# Install examples
install(TARGETS basic-server di-server
    RUNTIME DESTINATION bin/examples
)

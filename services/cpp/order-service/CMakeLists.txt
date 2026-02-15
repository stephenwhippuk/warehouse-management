cmake_minimum_required(VERSION 3.20)
project(order-service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Poco REQUIRED COMPONENTS Net NetSSL Util Foundation)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/Server.cpp
    src/models/Order.cpp
    src/dtos/ErrorDto.cpp
    src/dtos/OrderDto.cpp
    src/dtos/OrderListDto.cpp
    src/controllers/OrderController.cpp
    src/controllers/HealthController.cpp
    src/controllers/ClaimsController.cpp
    src/services/OrderService.cpp
    src/repositories/OrderRepository.cpp
    src/utils/Auth.cpp
    src/utils/Config.cpp
    src/utils/Logger.cpp
    src/utils/DtoMapper.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Poco::Net
    Poco::NetSSL
    Poco::Util
    Poco::Foundation
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Enable testing
enable_testing()
add_subdirectory(tests)

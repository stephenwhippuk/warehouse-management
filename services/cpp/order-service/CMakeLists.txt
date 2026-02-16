cmake_minimum_required(VERSION 3.20)
project(order-service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add http-framework subdirectory
if(NOT DEFINED HTTP_FRAMEWORK_DIR)
    set(HTTP_FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared/http-framework")
endif()
add_subdirectory(${HTTP_FRAMEWORK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/http-framework)

# Add warehouse-messaging subdirectory
if(NOT DEFINED WAREHOUSE_MESSAGING_DIR)
    set(WAREHOUSE_MESSAGING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared/warehouse-messaging")
endif()
add_subdirectory(${WAREHOUSE_MESSAGING_DIR} ${CMAKE_CURRENT_BINARY_DIR}/warehouse-messaging)

# Add contract-plugin subdirectory
if(NOT DEFINED CONTRACT_PLUGIN_DIR)
    set(CONTRACT_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared/contract-plugin")
endif()
add_subdirectory(${CONTRACT_PLUGIN_DIR} ${CMAKE_CURRENT_BINARY_DIR}/contract-plugin)

# Find packages
find_package(Poco REQUIRED COMPONENTS Net NetSSL Util Foundation)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CONTRACT_PLUGIN_DIR}/include)
include_directories(${PostgreSQL_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/models/Order.cpp
    src/dtos/ErrorDto.cpp
    src/dtos/OrderDto.cpp
    src/dtos/OrderListDto.cpp
    src/controllers/OrderController.cpp
    src/controllers/HealthController.cpp
    src/services/OrderService.cpp
    src/repositories/OrderRepository.cpp
    src/utils/Auth.cpp
    src/utils/Config.cpp
    src/utils/Database.cpp
    src/utils/Logger.cpp
    src/utils/DtoMapper.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        http-framework
        warehouse-messaging
        contract-plugin
        Poco::Net
        Poco::NetSSL
        Poco::Util
        Poco::Foundation
        PostgreSQL::PostgreSQL
        pqxx
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Enable testing
# TODO: Fix tests to match current Order model API
# enable_testing()
# add_subdirectory(tests)

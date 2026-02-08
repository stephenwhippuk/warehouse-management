cmake_minimum_required(VERSION 3.20)
project(inventory-service VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Poco REQUIRED COMPONENTS Net NetSSL Util Foundation)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# RabbitMQ C client (installed via apt: librabbitmq-dev)
find_library(RABBITMQ_LIBRARY NAMES rabbitmq)
if(NOT RABBITMQ_LIBRARY)
    message(FATAL_ERROR "rabbitmq-c library not found. Please install librabbitmq-dev.")
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/Server.cpp
    src/models/Inventory.cpp
    src/controllers/InventoryController.cpp
    src/controllers/HealthController.cpp
    src/controllers/SwaggerController.cpp
    src/repositories/InventoryRepository.cpp
    src/services/InventoryService.cpp
    src/utils/Database.cpp
    src/utils/Logger.cpp
    src/utils/Config.cpp
    src/utils/Auth.cpp
    src/utils/RabbitMqMessageBus.cpp
    src/utils/JsonValidator.cpp
    src/utils/SwaggerGenerator.cpp
    src/utils/ContractReader.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    Poco::Net
    Poco::NetSSL
    Poco::Util
    Poco::Foundation
    ${PostgreSQL_LIBRARIES}
    pqxx
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ${RABBITMQ_LIBRARY}
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY config/
    DESTINATION etc/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.json"
)

install(DIRECTORY contracts/
    DESTINATION share/${PROJECT_NAME}/contracts
    FILES_MATCHING PATTERN "*.json"
)

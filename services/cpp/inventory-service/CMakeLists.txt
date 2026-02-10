cmake_minimum_required(VERSION 3.20)
project(inventory-service VERSION 1.0.0 LANGUAGES CXX)

# Options
option(VALIDATE_CONTRACTS "Run contract validation after build" ON)

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
find_package(contract-validator REQUIRED)

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
    src/controllers/ClaimsController.cpp
    src/repositories/InventoryRepository.cpp
    src/services/InventoryService.cpp
    src/utils/Database.cpp
    src/utils/Logger.cpp
    src/utils/Config.cpp
    src/utils/Auth.cpp
    src/utils/RabbitMqMessageBus.cpp
    src/utils/JsonValidator.cpp
    src/utils/SwaggerGenerator.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Contract validator tool
add_executable(validate-contracts
    src/validate_contracts.cpp
    src/utils/Logger.cpp
    src/utils/Config.cpp
)

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
    ContractValidator::contract-validator
)

# Link libraries for validate-contracts tool
target_link_libraries(validate-contracts
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ContractValidator::contract-validator
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

# Contract validation post-build step (optional)
if(VALIDATE_CONTRACTS)
    message(STATUS "Contract validation enabled (disable with -DVALIDATE_CONTRACTS=OFF)")
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "========================================================================"
        COMMAND ${CMAKE_COMMAND} -E echo "Running contract validation..."
        COMMAND ${CMAKE_COMMAND} -E echo "========================================================================"
        COMMAND bash -c "validate-contracts --contracts-root ${PROJECT_SOURCE_DIR}/../../contracts --service-contracts ${PROJECT_SOURCE_DIR}/contracts --claims ${PROJECT_SOURCE_DIR}/claims.json > ${CMAKE_BINARY_DIR}/contract-validation-report.txt 2>&1 || true"
        COMMAND bash -c "validate-contracts --contracts-root ${PROJECT_SOURCE_DIR}/../../contracts --service-contracts ${PROJECT_SOURCE_DIR}/contracts --claims ${PROJECT_SOURCE_DIR}/claims.json --json 2>/dev/null > ${CMAKE_BINARY_DIR}/contract-validation-report.json || true"
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Contract validation complete!"
        COMMAND ${CMAKE_COMMAND} -E echo "Reports saved to:"
        COMMAND ${CMAKE_COMMAND} -E echo "  Text:  ${CMAKE_BINARY_DIR}/contract-validation-report.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "  JSON:  ${CMAKE_BINARY_DIR}/contract-validation-report.json"
        COMMAND ${CMAKE_COMMAND} -E echo "========================================================================"
        COMMENT "Validating contracts against service claims"
        VERBATIM
    )
else()
    message(STATUS "Contract validation disabled")
endif()

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

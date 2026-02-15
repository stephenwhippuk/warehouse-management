cmake_minimum_required(VERSION 3.20)
project(product-service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add warehouse-messaging library path
set(WAREHOUSE_MESSAGING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared/warehouse-messaging")
add_subdirectory(${WAREHOUSE_MESSAGING_DIR} ${CMAKE_CURRENT_BINARY_DIR}/warehouse-messaging)

# Find packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Poco REQUIRED COMPONENTS Net)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(Catch2 QUIET)  # Optional - only for tests

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Sources
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/Server.cpp
    src/models/Product.cpp
    src/dtos/ProductItemDto.cpp
    src/dtos/ProductListDto.cpp
    src/dtos/ErrorDto.cpp
    src/repositories/ProductRepository.cpp
    src/services/ProductService.cpp
    src/controllers/ProductController.cpp
    src/controllers/HealthController.cpp
    src/controllers/SwaggerController.cpp
    src/utils/Config.cpp
    src/utils/Logger.cpp
    src/utils/Database.cpp
    src/utils/Auth.cpp
    src/utils/DtoMapper.cpp
    src/utils/SwaggerGenerator.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    warehouse-messaging
    Boost::system
    Boost::thread
    Poco::Net
    pqxx
    pq
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    uuid
)

# Tests (optional - only if Catch2 is found)
if(Catch2_FOUND)
    enable_testing()

    set(TEST_SOURCES
        tests/test_main.cpp
        tests/ProductTests.cpp
        tests/DtoMapperTests.cpp
    )

    set(TEST_DTO_SOURCES
        src/dtos/ProductItemDto.cpp
        src/dtos/ProductListDto.cpp
        src/dtos/ErrorDto.cpp
        src/utils/DtoMapper.cpp
    )

    set(TEST_MODEL_SOURCES
        src/models/Product.cpp
    )

    add_executable(${PROJECT_NAME}-tests ${TEST_SOURCES} ${TEST_MODEL_SOURCES} ${TEST_DTO_SOURCES})

    target_link_libraries(${PROJECT_NAME}-tests
        Boost::system
        nlohmann_json::nlohmann_json
        Catch2::Catch2WithMain
        uuid
    )

    add_test(NAME ProductTests COMMAND ${PROJECT_NAME}-tests)
else()
    message(STATUS "Catch2 not found - skipping test build")
endif()

cmake_minimum_required(VERSION 3.20)
project(contract-plugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Required for shared library

# Find dependencies
find_package(nlohmann_json REQUIRED)
find_package(Poco REQUIRED COMPONENTS Foundation Net)
find_package(spdlog REQUIRED)

# Include http-framework
set(HTTP_FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../http-framework")
if(NOT EXISTS ${HTTP_FRAMEWORK_DIR})
    message(FATAL_ERROR "http-framework directory not found at ${HTTP_FRAMEWORK_DIR}")
endif()

# Source files
set(SOURCES
    src/ContractConfig.cpp
    src/ContractValidationMiddleware.cpp
    src/ClaimsLoader.cpp
    src/ClaimsService.cpp
    src/ClaimsController.cpp
    src/SwaggerService.cpp
    src/SwaggerController.cpp
    src/ContractPlugin.cpp
)

# Include directories
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${HTTP_FRAMEWORK_DIR}/include
)

# Build as shared library (.so)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        nlohmann_json::nlohmann_json
        Poco::Foundation
        Poco::Net
        spdlog::spdlog
)

# Installation
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/contract-plugin
    DESTINATION include
)

# Print configuration
message(STATUS "")
message(STATUS "========== Configuration Summary ==========")
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build shared library: ON")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "HTTP Framework: ${HTTP_FRAMEWORK_DIR}")
message(STATUS "==========================================")
message(STATUS "")

# ============================================================================
# Tests
# ============================================================================

# Only build tests if this is the main project (not a subdirectory)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(BUILD_TESTS_DEFAULT ON)
else()
    set(BUILD_TESTS_DEFAULT OFF)
endif()

option(BUILD_TESTS "Build tests" ${BUILD_TESTS_DEFAULT})

if(BUILD_TESTS)
    message(STATUS "Building tests...")
    
    # Find Catch2
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        message(STATUS "Catch2 not found, fetching from GitHub...")
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
        )
        FetchContent_MakeAvailable(Catch2)
    endif()
    
    # Test sources
    set(TEST_SOURCES
        tests/test_main.cpp
        tests/ClaimsServiceTests.cpp
        tests/SwaggerServiceTests.cpp
    )
    
    # Sources needed for tests (non-plugin specific)
    set(TEST_LIB_SOURCES
        src/ContractConfig.cpp
        src/ClaimsLoader.cpp
        src/ClaimsService.cpp
        src/SwaggerService.cpp
    )
    
    # Create test executable
    add_executable(contract-plugin-tests ${TEST_SOURCES} ${TEST_LIB_SOURCES})
    
    target_include_directories(contract-plugin-tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
            ${HTTP_FRAMEWORK_DIR}/include
    )
    
    target_link_libraries(contract-plugin-tests
        PRIVATE
            Catch2::Catch2WithMain
            nlohmann_json::nlohmann_json
            spdlog::spdlog
    )
    
    # Enable testing
    enable_testing()
    
    # Register tests with CTest
    add_test(NAME ClaimsServiceTests COMMAND contract-plugin-tests "[claims-service]")
    add_test(NAME SwaggerServiceTests COMMAND contract-plugin-tests "[swagger-service]")
    
    message(STATUS "Tests enabled: contract-plugin-tests")
else()
    message(STATUS "Tests disabled (set BUILD_TESTS=ON to enable)")
endif()

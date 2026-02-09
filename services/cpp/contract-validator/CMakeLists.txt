cmake_minimum_required(VERSION 3.20)
project(contract-validator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_EXECUTABLE "Build validate-contracts executable" ON)
option(BUILD_TESTS "Build tests" OFF)

# Find required packages
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# Library sources
set(LIBRARY_SOURCES
    src/ContractReader.cpp
    src/ContractValidator.cpp
    src/CppCodeParser.cpp
    src/CodeValidator.cpp
    src/Logger.cpp
)

# Library headers
set(LIBRARY_HEADERS
    include/contract_validator/ContractReader.hpp
    include/contract_validator/ContractValidator.hpp
    include/contract_validator/CppCodeParser.hpp
    include/contract_validator/CodeValidator.hpp
)

# Create library
add_library(contract-validator ${LIBRARY_SOURCES})

# Set include directories
target_include_directories(contract-validator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(contract-validator
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Set library properties
set_target_properties(contract-validator PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${LIBRARY_HEADERS}"
)

# Executable
if(BUILD_EXECUTABLE)
    add_executable(validate-contracts
        src/validate_contracts_main.cpp
    )
    
    target_link_libraries(validate-contracts
        PRIVATE
            contract-validator
    )
    
    install(TARGETS validate-contracts
        RUNTIME DESTINATION bin
    )
endif()

# Installation
install(TARGETS contract-validator
    EXPORT contract-validator-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/contract_validator
)

# Install cmake config files
install(EXPORT contract-validator-targets
    FILE contract-validator-targets.cmake
    NAMESPACE ContractValidator::
    DESTINATION lib/cmake/contract-validator
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/contract-validator-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/contract-validator-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/contract-validator-config.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/contract-validator-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/contract-validator-config-version.cmake"
    DESTINATION lib/cmake/contract-validator
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

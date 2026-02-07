cmake_minimum_required(VERSION 3.20)
project(warehouse-service VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(Poco REQUIRED COMPONENTS Net NetSSL Util Foundation)

# Optional: Redis
find_package(redis++ QUIET)
find_package(hiredis QUIET)

# Optional: JSON Schema validator
find_package(nlohmann_json_schema_validator QUIET)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/Server.cpp
    src/models/Warehouse.cpp
    src/models/Location.cpp
    src/controllers/WarehouseController.cpp
    src/controllers/LocationController.cpp
    src/controllers/SwaggerController.cpp
    src/repositories/WarehouseRepository.cpp
    src/repositories/LocationRepository.cpp
    src/services/WarehouseService.cpp
    src/services/LocationService.cpp
    src/utils/Database.cpp
    src/utils/Logger.cpp
    src/utils/Config.cpp
    src/utils/JsonValidator.cpp
    src/utils/SwaggerGenerator.cpp
)

# Header files
set(HEADERS
    include/warehouse/Application.hpp
    include/warehouse/Server.hpp
    include/warehouse/models/Warehouse.hpp
    include/warehouse/models/Location.hpp
    include/warehouse/models/Common.hpp
    include/warehouse/controllers/WarehouseController.hpp
    include/warehouse/controllers/LocationController.hpp
    include/warehouse/controllers/SwaggerController.hpp
    include/warehouse/repositories/WarehouseRepository.hpp
    include/warehouse/repositories/LocationRepository.hpp
    include/warehouse/services/WarehouseService.hpp
    include/warehouse/services/LocationService.hpp
    include/warehouse/utils/Database.hpp
    include/warehouse/utils/Logger.hpp
    include/warehouse/utils/Config.hpp
    include/warehouse/utils/JsonValidator.hpp
    include/warehouse/utils/SwaggerGenerator.hpp
)

# Create executable
add_executable(warehouse-service ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(warehouse-service
    PRIVATE
        Boost::system
        Boost::thread
        PostgreSQL::PostgreSQL
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Poco::Net
        Poco::NetSSL
        Poco::Util
        Poco::Foundation
)

# Link optional libraries
if(redis++_FOUND AND hiredis_FOUND)
    target_link_libraries(warehouse-service PRIVATE redis++ hiredis)
    target_compile_definitions(warehouse-service PRIVATE HAVE_REDIS)
endif()

if(nlohmann_json_schema_validator_FOUND)
    target_link_libraries(warehouse-service PRIVATE nlohmann_json_schema_validator)
    target_compile_definitions(warehouse-service PRIVATE HAVE_JSON_SCHEMA_VALIDATOR)
endif()

# Install targets
install(TARGETS warehouse-service
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY config/ DESTINATION etc/warehouse-service)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Warehouse Service Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Boost Version: ${Boost_VERSION}")
message(STATUS "  PostgreSQL: ${PostgreSQL_FOUND}")
message(STATUS "  Redis++: ${redis++_FOUND}")
message(STATUS "  JSON Schema Validator: ${nlohmann_json_schema_validator_FOUND}")
message(STATUS "")

cmake_minimum_required(VERSION 3.20)
project(http-framework VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Poco REQUIRED COMPONENTS Net NetSSL Util Foundation)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)

# Source files
set(SOURCES
    src/HttpContext.cpp
    src/Middleware.cpp
    src/ExceptionFilter.cpp
    src/Router.cpp
    src/ControllerBase.cpp
    src/HttpHost.cpp
    src/ServiceProvider.cpp
    src/ServiceScopeMiddleware.cpp
    src/PluginManager.cpp
)

# Headers
set(HEADERS
    include/http-framework/HttpContext.hpp
    include/http-framework/HttpException.hpp
    include/http-framework/ExceptionFilter.hpp
    include/http-framework/Middleware.hpp
    include/http-framework/Router.hpp
    include/http-framework/ControllerBase.hpp
    include/http-framework/HttpHost.hpp
    include/http-framework/ServiceLifetime.hpp
    include/http-framework/IServiceProvider.hpp
    include/http-framework/IServiceScope.hpp
    include/http-framework/ServiceDescriptor.hpp
    include/http-framework/ServiceCollection.hpp
    include/http-framework/ServiceNamespace.hpp
    include/http-framework/ServiceScopeMiddleware.hpp
    include/http-framework/IPlugin.hpp
    include/http-framework/NamespacedServiceCollection.hpp
    include/http-framework/PluginManager.hpp
)

# Create library
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Poco::Net
        Poco::Util
        Poco::Foundation
        nlohmann_json::nlohmann_json
        fmt::fmt
        dl  # For dlopen, dlsym, dlclose
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

# Export library properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${HEADERS}"
)

# Optional: Build examples
option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Optional: Build tests (only when built as main project, not as subdirectory)
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/http-framework
)

install(DIRECTORY include/http-framework
    DESTINATION include
)

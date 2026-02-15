# Shared C++ Libraries

This directory contains shared libraries used across all C++ services in the warehouse management system.

## Libraries

### 1. HTTP Framework

**Location**: `http-framework/`

A modern HTTP server framework inspired by ASP.NET Core, providing:

- **Middleware Pipeline**: Composable middleware for cross-cutting concerns (logging, auth, CORS, error handling)
- **Controller-Based Routing**: Declarative route registration with automatic parameter extraction
- **Type-Safe Routing**: Route constraints (UUID, int, alpha, alphanumeric)
- **Unified Request/Response Handling**: HttpContext abstraction with helper methods
- **Reduced Boilerplate**: ~60% less code compared to manual Poco HTTP handling

**Quick Example**:
```cpp
class InventoryController : public http::ControllerBase {
public:
    InventoryController(auto service) 
        : ControllerBase("/api/v1/inventory"), service_(service) {
        
        Get("/", [this](http::HttpContext& ctx) {
            return getAllItems(ctx);
        });
        
        Get("/{id:uuid}", [this](http::HttpContext& ctx) {
            return getById(ctx);
        });
        
        Post("/{id:uuid}/reserve", [this](http::HttpContext& ctx) {
            return reserve(ctx);
        });
    }
};

// Setup
http::HttpHost host(8080);
host.use(std::make_shared<http::LoggingMiddleware>());
host.use(std::make_shared<http::AuthenticationMiddleware>(apiKey));
host.addController(std::make_shared<InventoryController>(service));
host.start();
```

**Documentation**:
- [README](http-framework/README.md) - Getting started and quick reference
- [ARCHITECTURE](http-framework/ARCHITECTURE.md) - Detailed architecture and design
- [MIGRATION_GUIDE](http-framework/MIGRATION_GUIDE.md) - How to migrate existing services

**Features**:
- ✅ Middleware pipeline with composable middleware
- ✅ Route parameter extraction (`/inventory/{id}`)
- ✅ Route constraints (`{id:uuid}`, `{page:int}`)
- ✅ Query parameter parsing
- ✅ JSON request/response helpers
- ✅ Built-in error handling
- ✅ Authentication middleware
- ✅ CORS middleware
- ✅ Logging middleware

**Usage in Services**:
- `inventory-service`: ✅ Ready to migrate
- `warehouse-service`: ✅ Ready to migrate
- `product-service`: ✅ Ready to migrate
- `order-service`: ✅ Ready to migrate

### 2. Warehouse Messaging

**Location**: `warehouse-messaging/`

Common messaging infrastructure for RabbitMQ-based event-driven communication.

**Features**:
- Event publishers and subscribers
- Message serialization/deserialization
- Connection management
- Error handling and retries

**Documentation**: See [warehouse-messaging/README.md](warehouse-messaging/README.md)

## Building Shared Libraries

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler (GCC 11+ or Clang 13+)
- Poco C++ Libraries (Net, NetSSL, Util, Foundation)
- nlohmann/json
- (Optional) Catch2 for tests

### Build All Libraries

```bash
cd services/cpp/shared
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Build Individual Library

```bash
cd services/cpp/shared/http-framework
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Run Tests

```bash
cd build
ctest --output-on-failure
```

## Using Shared Libraries in Services

### CMakeLists.txt Integration

```cmake
# Add http-framework
add_subdirectory(${CMAKE_SOURCE_DIR}/../shared/http-framework http-framework)

# Link against library
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        http-framework
        # ... other dependencies
)
```

### Header Inclusion

```cpp
#include "http-framework/HttpHost.hpp"
#include "http-framework/ControllerBase.hpp"
#include "http-framework/Middleware.hpp"
```

## Development Guidelines

### Adding New Shared Library

1. Create directory under `services/cpp/shared/`
2. Follow standard CMake project structure:
   ```
   library-name/
     ├── CMakeLists.txt
     ├── README.md
     ├── include/
     │   └── library-name/
     │       └── *.hpp
     ├── src/
     │   └── *.cpp
     ├── tests/
     │   └── *Tests.cpp
     └── examples/
         └── *.cpp
   ```
3. Add library to root CMakeLists.txt (if exists)
4. Document API and usage patterns
5. Write comprehensive tests

### Code Standards

- **C++ Version**: C++20
- **Naming**: 
  - Classes: PascalCase
  - Functions/methods: camelCase
  - Variables: snake_case (private members with trailing `_`)
  - Namespaces: lowercase
- **Documentation**: Use Doxygen-style comments
- **Testing**: Catch2 framework, aim for >80% coverage
- **Error Handling**: Use exceptions for errors, std::optional for not-found

### Dependency Management

Shared libraries should:
- ✅ Minimize external dependencies
- ✅ Use standard library where possible
- ✅ Document all required dependencies
- ✅ Provide CMake find_package scripts
- ❌ Avoid circular dependencies
- ❌ Avoid service-specific logic

## Versioning

Shared libraries follow semantic versioning:
- **Major**: Breaking changes
- **Minor**: New features, backward compatible
- **Patch**: Bug fixes

Example: `1.2.3`

## CI/CD

Shared libraries are built and tested in CI:

```yaml
# .github/workflows/shared-libraries.yml
name: Shared Libraries
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libpoco-dev nlohmann-json3-dev
      - name: Build
        run: |
          cd services/cpp/shared
          mkdir build && cd build
          cmake ..
          make -j$(nproc)
      - name: Test
        run: |
          cd services/cpp/shared/build
          ctest --output-on-failure
```

## Support

For issues or questions about shared libraries:
1. Check library-specific documentation
2. Review examples in `examples/` directories
3. See migration guides for integration patterns
4. Open issue in project repository

## Roadmap

### HTTP Framework
- [ ] WebSocket support
- [ ] Custom route constraints registration
- [ ] OpenAPI generation from routes
- [ ] Rate limiting middleware
- [ ] Request/response interceptors

### General
- [ ] Shared configuration management library
- [ ] Shared metrics/telemetry library
- [ ] Shared database utilities
- [ ] Shared validation library
- [ ] Shared authentication/authorization library

## Contributing

When contributing to shared libraries:

1. **Maintain backward compatibility**: Breaking changes require major version bump
2. **Write tests**: All new features must have tests
3. **Document thoroughly**: Update README and add inline documentation
4. **Consider impact**: Changes affect multiple services
5. **Review carefully**: Shared code has higher quality bar

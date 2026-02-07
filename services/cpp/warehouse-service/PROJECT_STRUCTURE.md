# Warehouse Service - Project Structure

## Directory Tree

```
warehouse-service/
├── CMakeLists.txt                    # Main CMake configuration
├── Dockerfile                        # Docker image build
├── docker-compose.yml               # Docker compose setup with PostgreSQL/Redis
├── README.md                        # Comprehensive documentation
├── .gitignore                       # Git ignore rules
│
├── config/                          # Configuration files
│   ├── application.json             # Application configuration
│   └── .env.example                # Environment variables template
│
├── include/warehouse/              # Public headers
│   ├── Application.hpp             # Main application class
│   ├── Server.hpp                  # HTTP server wrapper
│   │
│   ├── models/                     # Domain models
│   │   ├── Common.hpp              # Common types (Address, Dimensions, etc.)
│   │   ├── Warehouse.hpp           # Warehouse entity
│   │   └── Location.hpp            # Location entity
│   │
│   ├── controllers/                # HTTP request handlers
│   │   ├── WarehouseController.hpp # Warehouse endpoints
│   │   └── LocationController.hpp  # Location endpoints
│   │
│   ├── repositories/               # Data access layer
│   │   ├── WarehouseRepository.hpp # Warehouse database operations
│   │   └── LocationRepository.hpp  # Location database operations
│   │
│   ├── services/                   # Business logic layer
│   │   ├── WarehouseService.hpp    # Warehouse business logic
│   │   └── LocationService.hpp     # Location business logic
│   │
│   └── utils/                      # Utility classes
│       ├── Database.hpp            # PostgreSQL connection
│       ├── Logger.hpp              # Logging wrapper (spdlog)
│       ├── Config.hpp              # Configuration management
│       └── JsonValidator.hpp       # JSON Schema validation
│
├── src/                            # Implementation files
│   ├── main.cpp                    # Entry point
│   ├── Application.cpp             # Application implementation
│   ├── Server.cpp                  # Server implementation
│   ├── STUBS.md                    # Documentation about stub implementations
│   │
│   ├── models/
│   │   ├── Common.cpp              # Common types implementation
│   │   ├── Warehouse.cpp           # Warehouse entity implementation
│   │   └── Location.cpp            # Location entity implementation
│   │
│   ├── controllers/
│   │   ├── WarehouseController.cpp # Warehouse controller (stub)
│   │   └── LocationController.cpp  # Location controller (stub)
│   │
│   ├── repositories/
│   │   ├── WarehouseRepository.cpp # Warehouse repository (stub)
│   │   └── LocationRepository.cpp  # Location repository (stub)
│   │
│   ├── services/
│   │   ├── WarehouseService.cpp    # Warehouse service (partial)
│   │   └── LocationService.cpp     # Location service (partial)
│   │
│   └── utils/
│       ├── Database.cpp            # Database implementation (partial)
│       ├── Logger.cpp              # Logger implementation (complete)
│       ├── Config.cpp              # Config implementation (complete)
│       └── JsonValidator.cpp       # Validator implementation (partial)
│
├── tests/                          # Test files
│   ├── CMakeLists.txt             # Test configuration
│   ├── test_main.cpp              # Catch2 main entry point
│   ├── WarehouseTests.cpp         # Warehouse model tests
│   └── LocationTests.cpp          # Location model tests
│
└── migrations/                     # Database migrations
    └── 001_init.sql               # Initial schema

```

## Layer Architecture

```
┌─────────────────────────────────────────────────┐
│              HTTP Requests                       │
└──────────────────┬──────────────────────────────┘
                   │
         ┌─────────▼─────────┐
         │   Server.cpp      │  Poco HTTPServer
         │  (Request Router) │
         └─────────┬─────────┘
                   │
    ┌──────────────┴──────────────┐
    │                             │
┌───▼────────────┐    ┌──────────▼────────┐
│  Controllers   │    │  Controllers      │
│  Warehouse     │    │  Location         │
└───┬────────────┘    └──────────┬────────┘
    │                             │
    │         Services Layer      │
    │    (Business Logic)         │
┌───▼────────────┐    ┌──────────▼────────┐
│  Services      │    │  Services         │
│  Warehouse     │    │  Location         │
└───┬────────────┘    └──────────┬────────┘
    │                             │
    │       Repository Layer      │
    │    (Data Access)            │
┌───▼────────────┐    ┌──────────▼────────┐
│  Repositories  │    │  Repositories     │
│  Warehouse     │    │  Location         │
└───┬────────────┘    └──────────┬────────┘
    │                             │
    └──────────────┬──────────────┘
                   │
         ┌─────────▼─────────┐
         │   Database.cpp    │
         │   (PostgreSQL)    │
         └───────────────────┘
```

## Key Features

### ✅ Implemented
- **Project structure** with proper separation of concerns
- **Domain models** (Warehouse, Location) matching JSON Schema contracts
- **CMake build system** with dependency management
- **Docker support** with multi-stage builds
- **Configuration management** (JSON + environment variables)
- **Logging** with spdlog integration
- **Database layer** structure with pqxx
- **HTTP server** scaffolding with Poco
- **Unit tests** framework with Catch2
- **Database migrations** SQL scripts

### 🚧 Stub/Partial Implementation
- Controllers (routing complete, handlers stubbed)
- Repositories (interface complete, queries stubbed)
- Services (validation complete, business logic partial)
- JSON Schema validation (structure complete, needs implementation)

### 📝 TODO for Full Implementation
1. **Database Operations**: Implement CRUD queries in repositories
2. **HTTP Handlers**: Complete request/response handling in controllers
3. **Business Logic**: Implement remaining service methods
4. **Validation**: Complete JSON Schema validation integration
5. **Error Handling**: Add comprehensive error handling
6. **Connection Pooling**: Implement database connection pool
7. **Integration Tests**: Add end-to-end API tests
8. **Metrics**: Add Prometheus metrics endpoint
9. **Health Checks**: Implement health check endpoint
10. **Authentication**: Add JWT or API key authentication

## API Endpoints (Planned)

### Warehouses
- `GET    /api/v1/warehouses` - List all warehouses
- `GET    /api/v1/warehouses/:id` - Get warehouse by ID
- `POST   /api/v1/warehouses` - Create warehouse
- `PUT    /api/v1/warehouses/:id` - Update warehouse
- `DELETE /api/v1/warehouses/:id` - Delete warehouse

### Locations
- `GET    /api/v1/locations` - List all locations
- `GET    /api/v1/locations/:id` - Get location by ID
- `GET    /api/v1/warehouses/:id/locations` - Get locations by warehouse
- `POST   /api/v1/locations` - Create location
- `PUT    /api/v1/locations/:id` - Update location
- `DELETE /api/v1/locations/:id` - Delete location

## Dependencies

### Required
- **C++ 20** compiler (GCC 11+, Clang 13+, MSVC 2022+)
- **CMake** 3.20+
- **Boost** (system, thread)
- **Poco** (Net, NetSSL, Util, Foundation)
- **PostgreSQL** client library (libpqxx)
- **nlohmann/json** for JSON parsing
- **spdlog** for logging

### Optional
- **Catch2** or **Google Test** for testing
- **Redis++** for caching
- **JSON Schema Validator** for request validation

## Building

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install -y build-essential cmake libboost-all-dev \
    libpoco-dev libpq-dev nlohmann-json3-dev libspdlog-dev

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run
./bin/warehouse-service

# Test
ctest --output-on-failure
```

## Status

**Current State**: ✅ Compiles and runs (stub implementation)
**Next Step**: Implement repository database queries
**Estimated Completion**: 2-3 days for full implementation

See `src/STUBS.md` for detailed implementation status.

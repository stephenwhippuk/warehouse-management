# C++ Microservices

This directory contains high-performance C++ 20 microservices for the warehouse management system.

## Overview

C++ services handle performance-critical operations that require low latency and high throughput:
- Real-time inventory tracking
- Order processing and routing
- Warehouse location management
- API gateway and request routing

## Prerequisites

- C++ 20 compatible compiler (GCC 11+, Clang 13+, or MSVC 2022+)
- CMake 3.20+
- vcpkg or Conan for dependency management
- Docker (for containerization)

## Common Dependencies

Most C++ services use:
- **Boost**: Utility libraries
- **Poco**: Networking and HTTP
- **nlohmann/json**: JSON parsing
- **spdlog**: Logging
- **Catch2 or Google Test**: Testing
- **PostgreSQL client library**: Database access
- **Redis client library**: Caching

## Project Structure

```
cpp/
├── warehouse-service/     # ✅ Warehouse and location management
├── inventory-service/     # ✅ Stock levels and inventory tracking
├── order-service/         # 🚧 Order processing
├── api-gateway/           # 🚧 API Gateway service
├── common/                # Shared libraries and utilities
└── CMakeLists.txt        # Root CMake configuration
```

## Building Services

### Using CMake

Each service can be built independently:

```bash
cd services/cpp/{service-name}
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Using Docker

Build all services using Docker Compose from the project root:

```bash
docker-compose build inventory-service order-service warehouse-service
```

## Development Setup

1. **Install dependencies** (using vcpkg):
```bash
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install boost poco nlohmann-json spdlog catch2
```

2. **Configure CMake**:
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

3. **Build**:
```bash
cmake --build build -j$(nproc)
```

4. **Run tests**:
```bash
cd build && ctest
```

## Coding Standards

- Follow C++ 20 best practices
- Use modern C++ features (smart pointers, RAII, move semantics)
- Follow the Google C++ Style Guide with modifications
- Use `clang-format` for code formatting
- Use `clang-tidy` for static analysis

## Testing

Each service includes:
- Unit tests (Catch2 or Google Test)
- Integration tests
- Performance benchmarks

Run tests:
```bash
cd build
ctest --output-on-failure
```

## Service Templates

New services should follow this structure:

```
service-name/
├── src/
│   ├── main.cpp
│   ├── service.cpp
│   └── handlers/
├── include/
│   ├── service.h
│   └── handlers/
├── tests/
│   ├── unit/
│   └── integration/
├── CMakeLists.txt
├── Dockerfile
└── README.md
```

## Configuration

Services are configured via:
- Environment variables
- Configuration files (JSON/YAML)
- Command-line arguments

Example configuration:
```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 8080
  },
  "database": {
    "host": "postgres",
    "port": 5432,
    "database": "warehouse_db"
  },
  "redis": {
    "host": "redis",
    "port": 6379
  }
}
```

## Logging

All services use structured logging with spdlog:
- Log levels: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL
- JSON formatted logs for production
- Console output for development

## Monitoring

Services expose metrics via:
- Prometheus metrics endpoint
- Health check endpoint (`/health`)
- Readiness endpoint (`/ready`)

## API Documentation

All C++ services expose OpenAPI 3.0 specifications at `/api/swagger.json` for API documentation and client generation:

```bash
# Access Swagger JSON
curl http://localhost:8080/api/swagger.json

# View in Swagger UI
docker run -p 8080:8080 -e SWAGGER_JSON_URL=http://host.docker.internal:8080/api/swagger.json swaggerapi/swagger-ui
```

**Key Features:**
- **Contract-First Development**: OpenAPI specs generated from C++ models
- **Client Generation**: Generate TypeScript, C#, Python, etc. clients
- **Interactive Testing**: Works with Swagger UI, Postman, Bruno
- **Service Integration**: Services can consume each other's APIs

**Implementation:**
- `SwaggerGenerator` utility generates OpenAPI 3.0 JSON
- `SwaggerController` serves specification at `/api/swagger.json`
- Schemas match JSON Schema contracts in `/contracts/schemas/v1/`

See [docs/cpp-swagger-openapi.md](../../docs/cpp-swagger-openapi.md) for detailed usage guide.

## Creating a New Service

1. Copy the service template
2. Update CMakeLists.txt with source files
3. Implement models matching JSON Schemas
4. Implement service logic (repositories, services, controllers)
5. Add SwaggerGenerator and SwaggerController
6. Document API with OpenAPI endpoint
7. Add unit and integration tests
8. Set up database migrations (see below)
9. Create Dockerfile with Sqitch
10. Update docker-compose.yml
11. Document service in README

## Database Migrations

C++ services use **Sqitch** for database schema migrations, providing a code-first agile development experience similar to Entity Framework.

### Quick Start

```bash
cd services/cpp/{service-name}

# Initialize Sqitch
sqitch init service_name --engine pg

# Create a migration
sqitch add 001_initial_schema -n "Create initial tables"

# Edit the migration files:
# - migrations/deploy/001_initial_schema.sql
# - migrations/revert/001_initial_schema.sql  
# - migrations/verify/001_initial_schema.sql

# Apply migrations
sqitch deploy

# Check status
sqitch status
```

### Example Migration

**deploy/001_initial_schema.sql:**
```sql
BEGIN;

CREATE TABLE items (
    id BIGSERIAL PRIMARY KEY,
    sku VARCHAR(100) NOT NULL UNIQUE,
    name VARCHAR(255) NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_items_sku ON items(sku);

COMMIT;
```

**revert/001_initial_schema.sql:**
```sql
BEGIN;

DROP INDEX IF EXISTS idx_items_sku;
DROP TABLE IF EXISTS items;

COMMIT;
```

### Integration in C++ Code

Services automatically run migrations on startup:

```cpp
#include "database/migration_runner.hpp"

int main() {
    warehouse::database::MigrationRunner migrator(db_url);
    
    if (migrator.needsMigration()) {
        migrator.applyMigrations();
    }
    
    // Continue service startup...
}
```

### Key Features

- **Version Control**: Migrations tracked in Git
- **Rollback Support**: Every migration has a revert script
- **Dependency Tracking**: Migrations can depend on others
- **Verification**: Test scripts ensure migrations applied correctly
- **CI/CD Integration**: Automated migration in deployment pipeline

For complete migration guide including alternative tools (Flyway, custom frameworks) and code-first approaches, see:

📖 **[C++ Database Migrations Guide](../../docs/cpp-database-migrations.md)**

## Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Modern C++ Features](https://github.com/AnthonyCalandra/modern-cpp-features)
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/)

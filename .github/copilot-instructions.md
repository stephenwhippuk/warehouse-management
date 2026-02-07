# GitHub Copilot Instructions - Warehouse Management System

## Project Overview

This is a microservices-based warehouse management system with:
- **C++ Services**: High-performance backend services (C++20)
- **C# Services**: Business logic and integration services (.NET 8)
- **TypeScript/Vue.js**: Frontend applications
- **PostgreSQL**: Primary database
- **Redis**: Caching layer
- **Sqitch**: Database migration management

## C++ Project Guidelines

### Architecture

Follow a **clean layered architecture** for all C++ services:

```
Controllers (HTTP) → Services (Business Logic) → Repositories (Data Access) → Database
```

**Layer Responsibilities:**
- **Controllers**: Handle HTTP requests/responses, route to services, minimal logic
- **Services**: Business logic, validation, orchestration, transaction management
- **Repositories**: Database queries, CRUD operations, data mapping
- **Utils**: Cross-cutting concerns (logging, config, database connection, validation)

### Code Standards

**C++ Version**: C++20
- Use modern C++ features: `auto`, `constexpr`, structured bindings, concepts
- Prefer `std::optional` over nullable pointers
- Use `std::shared_ptr` for dependency injection
- Use `std::unique_ptr` for ownership
- Always use RAII for resource management

**Naming Conventions:**
- **Classes**: PascalCase (e.g., `WarehouseService`, `InventoryRepository`)
- **Functions/Methods**: camelCase (e.g., `findById`, `createWarehouse`)
- **Variables**: snake_case for private members with trailing underscore (e.g., `warehouse_id_`, `quantity_`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_CONNECTIONS`)
- **Namespaces**: lowercase (e.g., `warehouse::models`, `inventory::services`)

**File Organization:**
```
include/{service}/          # Public headers
  ├── models/              # Domain entities
  ├── controllers/         # HTTP handlers
  ├── services/            # Business logic
  ├── repositories/        # Data access
  └── utils/               # Utilities

src/                       # Implementation files
  ├── models/
  ├── controllers/
  ├── services/
  ├── repositories/
  └── utils/
```

### Dependencies

**Standard Libraries:**
- **Boost**: System utilities, threading
- **Poco**: HTTP server, networking (prefer over alternatives)
- **PostgreSQL (pqxx)**: Database client
- **nlohmann/json**: JSON parsing (always use `using json = nlohmann::json;`)
- **spdlog**: Logging (structured logging preferred)
- **Catch2**: Unit testing framework
- **OpenAPI 3.0**: API documentation (manual JSON generation)

**Include Order:**
```cpp
// 1. Corresponding header (if .cpp file)
#include "warehouse/models/Warehouse.hpp"

// 2. Project headers
#include "warehouse/utils/Logger.hpp"

// 3. Third-party libraries
#include <nlohmann/json.hpp>
#include <Poco/Net/HTTPServer.h>

// 4. Standard library
#include <string>
#include <memory>
#include <vector>
```

### Model Classes

All domain models must:
- Match JSON Schema contracts in `/contracts/schemas/v1/`
- Implement `toJson()` and `fromJson()` methods
- Use getters/setters for encapsulation
- Include business logic methods (e.g., `reserve()`, `isExpired()`)

**Example:**
```cpp
class Inventory {
public:
    // Constructors
    Inventory() = default;
    Inventory(const std::string& id, ...);
    
    // Getters (const methods)
    std::string getId() const { return id_; }
    int getQuantity() const { return quantity_; }
    
    // Setters
    void setQuantity(int quantity) { quantity_ = quantity; }
    
    // Business methods
    void reserve(int quantity);
    bool isExpired() const;
    
    // Serialization (always implement)
    json toJson() const;
    static Inventory fromJson(const json& j);
    
private:
    std::string id_;
    int quantity_ = 0;
};
```

### Repositories

**Pattern:**
```cpp
class InventoryRepository {
public:
    explicit InventoryRepository(std::shared_ptr<pqxx::connection> db);
    
    // CRUD operations
    std::optional<models::Inventory> findById(const std::string& id);
    std::vector<models::Inventory> findAll();
    models::Inventory create(const models::Inventory& inventory);
    models::Inventory update(const models::Inventory& inventory);
    bool deleteById(const std::string& id);
    
private:
    std::shared_ptr<pqxx::connection> db_;
};
```

**Database Queries:**
- Always use parameterized queries (prevent SQL injection)
- Use transactions for multi-statement operations
- Handle exceptions and convert to domain errors

```cpp
std::optional<models::Inventory> InventoryRepository::findById(const std::string& id) {
    pqxx::work txn(*db_);
    auto result = txn.exec_params(
        "SELECT * FROM inventory WHERE id = $1",
        id
    );
    txn.commit();
    
    if (result.empty()) {
        return std::nullopt;
    }
    
    return models::Inventory::fromJson(rowToJson(result[0]));
}
```

### Services

**Pattern:**
```cpp
class InventoryService {
public:
    explicit InventoryService(std::shared_ptr<repositories::InventoryRepository> repository);
    
    // Business operations
    std::optional<models::Inventory> getById(const std::string& id);
    models::Inventory create(const models::Inventory& inventory);
    
    // Validation
    bool isValidInventory(const models::Inventory& inventory) const;
    
private:
    std::shared_ptr<repositories::InventoryRepository> repository_;
};
```

**Best Practices:**
- Validate all inputs
- Throw meaningful exceptions (`std::invalid_argument`, `std::runtime_error`)
- Log important operations
- Keep services stateless

### Controllers

**Pattern:**
```cpp
class InventoryController : public Poco::Net::HTTPRequestHandler {
public:
    explicit InventoryController(std::shared_ptr<services::InventoryService> service);
    
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;
    
private:
    void handleGetById(const std::string& id, Response& response);
    void handleCreate(Request& request, Response& response);
    void sendJsonResponse(Response& response, const std::string& json, int status);
    void sendErrorResponse(Response& response, const std::string& msg, int status);
    
    std::shared_ptr<services::InventoryService> service_;
};
```

**HTTP Routing:**
- Parse URI path and method
- Extract path parameters (e.g., `/api/v1/inventory/:id`)
- Parse query parameters
- Parse JSON request body
- Return JSON responses with proper status codes

### API Documentation (Swagger/OpenAPI)

**Always document APIs** using OpenAPI 3.0 specification:

**Swagger Generator Utility:**
```cpp
class SwaggerGenerator {
public:
    static json generateSpec(const std::string& title, const std::string& version);
    static void addEndpoint(json& spec, const std::string& path, const std::string& method,
                           const std::string& summary, const json& requestBody,
                           const json& responses);
};
```

**Swagger Endpoint Pattern:**
```cpp
void handleSwagger(Poco::Net::HTTPServerResponse& response) {
    json spec = SwaggerGenerator::generateSpec("Inventory Service API", "1.0.0");
    
    // Add endpoints
    SwaggerGenerator::addEndpoint(spec, "/api/v1/inventory/{id}", "get",
        "Get inventory by ID",
        json(nullptr), // No request body
        {
            {"200", {{"description", "Success"}, {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/Inventory"}}}}}}}}}},
            {"404", {{"description", "Not found"}}}
        });
    
    sendJsonResponse(response, spec.dump(2), 200);
}
```

**OpenAPI Structure:**
- Place definitions in `docs/openapi.json` (generated)
- Serve at `/api/swagger.json` endpoint
- Include all schemas from `/contracts/schemas/v1/`
- Document all endpoints with request/response examples
- Use proper HTTP status codes (200, 201, 400, 404, 500)

**Documentation Best Practices:**
1. **Use JSON Schema references** - Reference contract schemas
2. **Document errors** - Include error response formats
3. **Provide examples** - Add example requests/responses
4. **Version APIs** - Use `/api/v1/` prefix
5. **Tags for grouping** - Organize endpoints by resource

### Error Handling

**Pattern:**
```cpp
try {
    auto inventory = service_->getById(id);
    if (!inventory) {
        sendErrorResponse(response, "Inventory not found", 404);
        return;
    }
    sendJsonResponse(response, inventory->toJson().dump(), 200);
} catch (const std::invalid_argument& e) {
    sendErrorResponse(response, e.what(), 400);
} catch (const std::exception& e) {
    utils::Logger::error("Internal error: {}", e.what());
    sendErrorResponse(response, "Internal server error", 500);
}
```

### Logging

Use **spdlog** for all logging:

```cpp
#include "inventory/utils/Logger.hpp"

// In code:
utils::Logger::info("Processing request for inventory {}", id);
utils::Logger::warn("Low stock detected: {}", productId);
utils::Logger::error("Database error: {}", e.what());
utils::Logger::debug("Query result: {} rows", result.size());
```

**Log Levels:**
- `debug`: Detailed debugging information
- `info`: Normal operational messages
- `warn`: Warning conditions
- `error`: Error conditions

### Testing

Use **Catch2** for unit tests:

```cpp
#include <catch2/catch_all.hpp>
#include "inventory/models/Inventory.hpp"

TEST_CASE("Inventory operations", "[inventory][operations]") {
    SECTION("Reserve inventory") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.reserve(30);
        
        REQUIRE(inv.getAvailableQuantity() == 70);
        REQUIRE(inv.getReservedQuantity() == 30);
    }
    
    SECTION("Reserve more than available throws exception") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        REQUIRE_THROWS_AS(inv.reserve(150), std::runtime_error);
    }
}
```

### Database Migrations

**Always use Sqitch** for database migrations:

```bash
# Add new migration
sqitch add 002_feature_name -n "Description of changes"

# Creates:
# migrations/deploy/002_feature_name.sql   # Forward migration
# migrations/revert/002_feature_name.sql   # Rollback script
# migrations/verify/002_feature_name.sql   # Verification tests
```

**Migration Rules:**
1. **Always provide rollback scripts** - Every deploy needs a revert
2. **Use transactions** - Wrap in BEGIN/COMMIT
3. **Make idempotent when possible** - Use IF NOT EXISTS
4. **Test rollback** - `sqitch deploy && sqitch revert && sqitch deploy`
5. **Never modify applied migrations** - Create new migration instead

**Example Deploy:**
```sql
-- Deploy inventory-service:002_add_alerts to pg
-- requires: 001_initial_schema

BEGIN;

CREATE TABLE inventory_alerts (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    inventory_id UUID NOT NULL REFERENCES inventory(id),
    alert_type VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_alerts_inventory ON inventory_alerts(inventory_id);

COMMIT;
```

**Example Revert:**
```sql
-- Revert inventory-service:002_add_alerts from pg

BEGIN;

DROP TABLE IF EXISTS inventory_alerts;

COMMIT;
```

**Example Verify:**
```sql
-- Verify inventory-service:002_add_alerts on pg

BEGIN;

SELECT id, inventory_id, alert_type, created_at
FROM inventory_alerts
WHERE FALSE;

ROLLBACK;
```

### CMake Configuration

**Project Structure:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(inventory-service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Poco REQUIRED COMPONENTS Net NetSSL Util Foundation)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# Sources
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/models/Inventory.cpp
    src/controllers/InventoryController.cpp
    src/repositories/InventoryRepository.cpp
    src/services/InventoryService.cpp
    src/utils/Database.cpp
    src/utils/Logger.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Boost::system
    Poco::Net
    PostgreSQL::PostgreSQL
    pqxx
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
```

### Docker Best Practices

**Multi-stage builds:**
```dockerfile
# Build stage
FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y build-essential cmake ...
COPY . .
RUN mkdir build && cd build && cmake .. && make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y sqitch libdbd-pg-perl ...
COPY --from=builder /app/build/bin/service /app/
COPY --from=builder /app/migrations /app/migrations
COPY docker-entrypoint.sh /app/
ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["./service"]
```

**Entrypoint Script:**
- Wait for database
- Run Sqitch migrations
- Start service

### Configuration

**Use environment variables** with fallback to config file:

```cpp
std::string dbUrl = utils::Config::getEnv("DATABASE_URL",
    utils::Config::getString("database.connectionString", "default"));
```

**Config File (application.json):**
```json
{
  "server": {
    "port": 8080,
    "host": "0.0.0.0"
  },
  "database": {
    "connectionString": "postgresql://user:pass@localhost:5432/db"
  },
  "logging": {
    "level": "info"
  }
}
```

### JSON Schema Validation

All requests should validate against contracts:

```cpp
#include "inventory/utils/JsonValidator.hpp"

bool InventoryService::create(const models::Inventory& inventory) {
    auto j = inventory.toJson();
    if (!utils::JsonValidator::validateInventory(j)) {
        throw std::invalid_argument("Invalid inventory data");
    }
    return repository_->create(inventory);
}
```

### Common Patterns

**Optional Values:**
```cpp
std::optional<std::string> getBatchNumber() const { return batchNumber_; }

// Usage:
if (inv.getBatchNumber()) {
    std::cout << *inv.getBatchNumber();
}
```

**Enum Conversions:**
```cpp
enum class Status { ACTIVE, INACTIVE };

std::string statusToString(Status status) {
    switch (status) {
        case Status::ACTIVE: return "active";
        case Status::INACTIVE: return "inactive";
    }
}

Status statusFromString(const std::string& str) {
    if (str == "active") return Status::ACTIVE;
    if (str == "inactive") return Status::INACTIVE;
    throw std::invalid_argument("Invalid status: " + str);
}
```

**JSON Serialization:**
```cpp
json Inventory::toJson() const {
    json j = {
        {"id", id_},
        {"quantity", quantity_}
    };
    
    // Optional fields
    if (batchNumber_) j["batchNumber"] = *batchNumber_;
    if (metadata_) j["metadata"] = *metadata_;
    
    return j;
}
```

### Documentation

**Header Comments:**
```cpp
/**
 * @brief Manages inventory stock levels and operations
 * 
 * Handles reserve, release, allocate, and deallocate operations
 * for warehouse inventory. Validates quantity relationships and
 * tracks inventory movements.
 */
class InventoryService {
    // ...
};
```

**Method Comments (when not obvious):**
```cpp
/**
 * @brief Reserves inventory quantity for an order
 * @param id Inventory record ID
 * @param quantity Amount to reserve
 * @throws std::runtime_error if insufficient available quantity
 */
void reserve(const std::string& id, int quantity);
```

### Performance Considerations

1. **Connection Pooling**: Implement for database connections
2. **Caching**: Use Redis for frequently accessed data
3. **Bulk Operations**: Batch database operations when possible
4. **Async I/O**: Consider for high-throughput scenarios
5. **Memory Management**: Use move semantics, avoid unnecessary copies

### Security

1. **Parameterized Queries**: Always use for SQL (prevents injection)
2. **Input Validation**: Validate all user inputs
3. **Error Messages**: Don't expose internal details
4. **Authentication**: Implement JWT or API key validation
5. **HTTPS**: Use TLS for production

### When Creating New Components

**New Service Checklist:**
1. Create directory structure (include/, src/, tests/, migrations/)
2. Set up CMakeLists.txt
3. Create Dockerfile and docker-compose.yml
4. Initialize Sqitch (sqitch.conf, sqitch.plan)
5. Create initial migration
6. Create models matching JSON schemas
7. Implement repositories (with TODOs if needed)
8. Implement services with validation
9. ImplemenSwaggerGenerator utility
11. Add `/api/swagger.json` endpoint
12. Create unit tests
13. Update README.md with API endpoints
14. Update README.md with API endpoints
12. Create PROJECT_STRUCTURE.md

**New Migration Checklist:**
1. Add with Sqitch: `sqitch add NNN_name -n "Description"`
2. Write deploy script (forward changes)
3. Write revert script (rollback changes)
4. Write verify script (test changes exist)
5. Test locally: deploy → verify → revert → deploy
6. Commit to Git with plan file

## General Best Practices

- **Prefer composition over inheritance**
- **Use dependency injection** (constructor injection)
- **Keep functions small** (single responsibility)
- **Write self-documenting code** (clear names over comments)
- **Test business logic** (unit tests for services)
- **Handle errors gracefully** (don't crash, log and return errors)
- **Use const correctness** (const methods, const parameters)
- **Avoid raw pointers** (use smart pointers)
- **RAII for resources** (always use constructors/destructors)

## References

- C++ Core Guidelines: https://isocpp.github.io/CppCoreGuidelines/
- Sqitch Documentation: https://sqitch.org/
- Poco Documentation: https://pocoproject.org/docs/
- Project Architecture: `/docs/architecture.md`
- Database Migrations: `/docs/cpp-database-migrations.md`
- Contracts: `/contracts/README.md`

# GitHub Copilot Instructions - Warehouse Management System

## Project Overview

This is a microservices-based warehouse management system with:
- **C++ Services**: High-performance backend services (C++20)
- **C# Services**: Business logic and integration services (.NET 8)
- **TypeScript/Vue.js**: Frontend applications
- **PostgreSQL**: Primary database
- **Redis**: Caching layer
- **Sqitch**: Database migration management

**IMPORTANT**: This project uses a comprehensive **Contract System** to ensure consistency across services. All services must declare contracts (fulfilments/references), define DTOs/Requests/Events/Endpoints, and maintain a `claims.json` manifest. See "Contract System" section below and `/contracts/docs/overview.md` for full details.

**CRITICAL ARCHITECTURE POLICY**: Services MUST return DTOs, NOT domain models. Domain models remain internal to the service/repository layers. DTOs are the external API contract. See "Data Transfer Objects (DTOs)" section and `/docs/dto-architecture-pattern.md` for complete implementation guide.

## C++ Project Guidelines

### Architecture

Follow a **clean layered architecture** for all C++ services:

```
Controllers (HTTP) → DTOs → Services (Business Logic) → Models → Repositories (Data Access) → Database
                      ↑                                    ↑
                 Public API                           Internal Only
```

**Layer Responsibilities:**
- **Controllers**: Handle HTTP requests/responses, work with DTOs, minimal logic
- **DTOs**: External API contracts, immutable, validated, entity-prefixed references
- **Services**: Business logic, validation, orchestration, convert models to DTOs
- **Models**: Domain entities, business logic, internal representation only
- **Repositories**: Database queries, CRUD operations, data mapping
- **Utils**: Cross-cutting concerns (logging, config, database connection, validation, DTO mapping)

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
  ├── models/              # Domain entities (internal)
  ├── dtos/                # Data Transfer Objects (external API)
  ├── controllers/         # HTTP handlers
  ├── services/            # Business logic
  ├── repositories/        # Data access
  └── utils/               # Utilities

src/                       # Implementation files
  ├── models/
  ├── dtos/
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
- Match entity contracts in `/contracts/entities/v1/` (see Contract System section)
- Implement `toJson()` and `fromJson()` methods
- Use getters/setters for encapsulation
- Include business logic methods (e.g., `reserve()`, `isExpired()`)
- Include all fields declared in the service's fulfilment claims
- **REMAIN INTERNAL** - Models are never exposed outside service/repository layers

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

### Data Transfer Objects (DTOs)

**CRITICAL ARCHITECTURE POLICY**: Services MUST return DTOs, NOT domain models. DTOs are the external API contract while models remain internal.

```
Controllers (HTTP) → DTOs → Services → Models → Repositories → Database
                      ↑
                  Public API
```

**DTO Requirements:**
- **Immutable**: All fields passed via constructor, no setters
- **Validated**: Constructor validates all fields using contractual type restrictions
- **Contract-compliant**: Must match DTO definitions in `/contracts/dtos/`
- **Entity-prefixed**: Referenced entity fields use prefixes (e.g., `ProductId`, `WarehouseName`)
- **Serializable**: Provide `toJson()` method for API responses
- **Performance**: Return collections by `const` reference, not by value (avoid expensive copies)

**DTO Pattern:**
```cpp
// Header: include/{service}/dtos/InventoryItemDto.hpp
class InventoryItemDto {
public:
    /**
     * @brief Construct DTO with all required fields - validates on construction
     */
    InventoryItemDto(const std::string& id,              // UUID
                     const std::string& productId,       // UUID
                     const std::string& productSku,      // Identity field
                     int quantity,                       // NonNegativeInteger
                     const std::string& status,          // InventoryStatus enum
                     const std::string& createdAt,       // DateTime (ISO 8601)
                     const std::optional<std::string>& serialNumber = std::nullopt);

    // Immutable getters (const, no setters)
    // Scalar values: return by value
    std::string getId() const { return id_; }
    std::string getProductId() const { return productId_; }
    std::string getProductSku() const { return productSku_; }
    int getQuantity() const { return quantity_; }
    
    // Collections: MUST return by const reference to avoid expensive copies
    // const std::vector<SomeType>& getItems() const { return items_; }
    
    // Serialization
    json toJson() const;

private:
    // Fields
    std::string id_;
    std::string productId_;
    std::string productSku_;
    int quantity_;
    std::string status_;
    std::string createdAt_;
    std::optional<std::string> serialNumber_;

    // Validation (called from constructor)
    void validateUuid(const std::string& uuid, const std::string& fieldName) const;
    void validateNonNegativeInteger(int value, const std::string& fieldName) const;
    void validateDateTime(const std::string& dateTime, const std::string& fieldName) const;
    void validateInventoryStatus(const std::string& status) const;
};
```

**DTO Implementation Pattern:**
```cpp
// src/dtos/InventoryItemDto.cpp
InventoryItemDto::InventoryItemDto(
    const std::string& id,
    const std::string& productId,
    const std::string& productSku,
    int quantity,
    const std::string& status,
    const std::string& createdAt,
    const std::optional<std::string>& serialNumber)
    : id_(id)
    , productId_(productId)
    , productSku_(productSku)
    , quantity_(quantity)
    , status_(status)
    , createdAt_(createdAt)
    , serialNumber_(serialNumber) {
    
    // Validate all fields using contractual type restrictions
    validateUuid(id_, "id");
    validateUuid(productId_, "ProductId");
    
    if (productSku_.empty()) {
        throw std::invalid_argument("ProductSku cannot be empty");
    }
    
    validateNonNegativeInteger(quantity_, "quantity");
    validateInventoryStatus(status_);
    validateDateTime(createdAt_, "createdAt");
}

json InventoryItemDto::toJson() const {
    json j = {
        {"id", id_},
        {"ProductId", productId_},
        {"ProductSku", productSku_},
        {"quantity", quantity_},
        {"status", status_},
        {"createdAt", createdAt_}
    };
    
    if (serialNumber_) {
        j["serialNumber"] = *serialNumber_;
    }
    
    return j;
}
```

**Common Validation Helpers:**
```cpp
// UUID validation (contract type: UUID)
void validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}

// NonNegativeInteger validation (contract type: NonNegativeInteger)
void validateNonNegativeInteger(int value, const std::string& fieldName) const {
    if (value < 0) {
        throw std::invalid_argument(fieldName + " must be non-negative");
    }
}

// PositiveInteger validation (contract type: PositiveInteger)
void validatePositiveInteger(int value, const std::string& fieldName) const {
    if (value < 1) {
        throw std::invalid_argument(fieldName + " must be positive");
    }
}

// DateTime validation (contract type: DateTime - ISO 8601)
void validateDateTime(const std::string& dateTime, const std::string& fieldName) const {
    if (dateTime.empty()) {
        throw std::invalid_argument(fieldName + " cannot be empty");
    }
    static const std::regex isoRegex(
        R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$)"
    );
    if (!std::regex_match(dateTime, isoRegex)) {
        throw std::invalid_argument(fieldName + " must be in ISO 8601 format");
    }
}
```

**DTO Mapper Utility:**
Create a mapper to convert models to DTOs:

```cpp
// include/{service}/utils/DtoMapper.hpp
class DtoMapper {
public:
    static dtos::InventoryItemDto toInventoryItemDto(
        const models::Inventory& inventory,
        const std::string& productSku,      // From Product reference
        const std::string& warehouseCode,   // From Warehouse reference
        const std::string& locationCode);   // From Location reference
};

// src/utils/DtoMapper.cpp
dtos::InventoryItemDto DtoMapper::toInventoryItemDto(
    const models::Inventory& inventory,
    const std::string& productSku,
    const std::string& warehouseCode,
    const std::string& locationCode) {
    
    std::string statusStr = inventoryStatusToLowerString(inventory);
    
    return dtos::InventoryItemDto(
        inventory.getId(),
        inventory.getProductId(),
        productSku,                         // Identity field from Product
        inventory.getWarehouseId(),
        warehouseCode,                      // Identity field from Warehouse
        inventory.getLocationId(),
        locationCode,                       // Identity field from Location
        inventory.getQuantity(),
        inventory.getReservedQuantity(),
        inventory.getAllocatedQuantity(),
        inventory.getAvailableQuantity(),
        statusStr,
        inventory.getCreatedAt().value_or(""),
        inventory.getUpdatedAt().value_or("")
    );
}
```

**Standard DTOs (create for every service):**

1. **ErrorDto**: Standard error response
   - Fields: `error`, `message`, `requestId`, `timestamp`, `path`, `details` (optional)

2. **EntityItemDto**: Single entity with referenced data
   - All fulfilled entity fields
   - Referenced entity identity fields (entity-prefixed)
   - Computed fields (e.g., `availableQuantity`)

3. **EntityListDto**: Paginated list response
   - Fields: `items`, `totalCount`, `page`, `pageSize`, `totalPages`
   - **CRITICAL**: `getItems()` must return `const std::vector<EntityDto>&` (by const reference)
   - Prevents expensive vector copies on every access

4. **OperationResultDto**: Result of mutating operations
   - Entity state after operation
   - Operation details: `operation`, `operationQuantity`, `success`, `message`

**EntityListDto Pattern (Correct Getter Implementation):**
```cpp
class InventoryListDto {
public:
    InventoryListDto(const std::vector<InventoryItemDto>& items,
                     int totalCount, int page, int pageSize, int totalPages);
    
    // ✅ CORRECT: Return collection by const reference (zero-cost access)
    const std::vector<InventoryItemDto>& getItems() const { return items_; }
    
    // ✅ CORRECT: Scalar values returned by value
    int getTotalCount() const { return totalCount_; }
    int getPage() const { return page_; }
    int getPageSize() const { return pageSize_; }
    int getTotalPages() const { return totalPages_; }
    
    json toJson() const;

private:
    std::vector<InventoryItemDto> items_;
    int totalCount_;
    int page_;
    int pageSize_;
    int totalPages_;
};

// ❌ WRONG: Returning collection by value (expensive copy on every call)
// std::vector<InventoryItemDto> getItems() const { return items_; }
```

**DTO Directory Structure:**
```
contracts/dtos/                 # Contract definitions (JSON)
  ├── ErrorDto.json
  ├── InventoryItemDto.json
  ├── InventoryListDto.json
  └── InventoryOperationResultDto.json

include/{service}/dtos/         # DTO headers
  ├── ErrorDto.hpp
  ├── InventoryItemDto.hpp
  ├── InventoryListDto.hpp
  └── InventoryOperationResultDto.hpp

src/dtos/                       # DTO implementations
  ├── ErrorDto.cpp
  ├── InventoryItemDto.cpp
  ├── InventoryListDto.cpp
  └── InventoryOperationResultDto.cpp
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

**Pattern (MUST return DTOs, not models):**
```cpp
class InventoryService {
public:
    explicit InventoryService(std::shared_ptr<repositories::InventoryRepository> repository);
    
    // Business operations - return DTOs, not models
    std::optional<dtos::InventoryItemDto> getById(const std::string& id);
    std::vector<dtos::InventoryItemDto> getAll();
    dtos::InventoryItemDto create(const models::Inventory& inventory);
    dtos::InventoryItemDto update(const models::Inventory& inventory);
    
    // Stock operations - return operation result DTOs
    dtos::InventoryOperationResultDto reserve(const std::string& id, int quantity);
    dtos::InventoryOperationResultDto release(const std::string& id, int quantity);
    
    // Validation
    bool isValidInventory(const models::Inventory& inventory) const;
    
private:
    std::shared_ptr<repositories::InventoryRepository> repository_;
};
```

**Service Implementation Pattern:**
```cpp
#include "inventory/utils/DtoMapper.hpp"

std::optional<dtos::InventoryItemDto> InventoryService::getById(const std::string& id) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        return std::nullopt;
    }
    
    // TODO: Fetch identity fields from Product, Warehouse, Location services
    // For now, using placeholders
    return utils::DtoMapper::toInventoryItemDto(
        *inventory,
        "SKU-" + inventory->getProductId().substr(0, 8),  // Placeholder
        "WH-" + inventory->getWarehouseId().substr(0, 8),
        "LOC-" + inventory->getLocationId().substr(0, 8)
    );
}

std::vector<dtos::InventoryItemDto> InventoryService::getAll() {
    auto inventories = repository_->findAll();
    std::vector<dtos::InventoryItemDto> dtos;
    dtos.reserve(inventories.size());
    
    for (const auto& inv : inventories) {
        // TODO: Batch fetch identity fields to improve performance
        dtos.push_back(utils::DtoMapper::toInventoryItemDto(
            inv,
            "SKU-" + inv.getProductId().substr(0, 8),
            "WH-" + inv.getWarehouseId().substr(0, 8),
            "LOC-" + inv.getLocationId().substr(0, 8)
        ));
    }
    
    return dtos;
}

dtos::InventoryOperationResultDto InventoryService::reserve(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->reserve(quantity);
    auto updated = repository_->update(*inventory);
    
    // Return operation result DTO
    return utils::DtoMapper::toInventoryOperationResultDto(
        updated, "reserve", quantity, true, std::nullopt
    );
}
```

**Best Practices:**
- **Always return DTOs** - Services are the boundary between internal models and external API
- Validate all inputs
- Throw meaningful exceptions (`std::invalid_argument`, `std::runtime_error`)
- Log important operations
- Keep services stateless
- Use DtoMapper to convert models to DTOs
- Add TODOs for fetching real reference data

### Controllers

**Pattern (work exclusively with DTOs):**
```cpp
class InventoryController : public Poco::Net::HTTPRequestHandler {
public:
    explicit InventoryController(std::shared_ptr<services::InventoryService> service);
    
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;
    
private:
    void handleGetById(const std::string& id, Response& response);
    void handleCreate(Request& request, Response& response);
    void handleReserve(const std::string& id, Request& request, Response& response);
    void sendJsonResponse(Response& response, const std::string& json, int status);
    void sendErrorResponse(Response& response, const std::string& msg, int status);
    
    std::shared_ptr<services::InventoryService> service_;
};
```

**Controller Implementation Pattern:**
```cpp
void InventoryController::handleGetAll(Response& response) {
    auto dtos = service_->getAll();  // Returns vector<InventoryItemDto>
    json j = json::array();
    for (const auto& dto : dtos) {
        j.push_back(dto.toJson());   // DTOs have toJson() method
    }
    sendJsonResponse(response, j.dump());
}

void InventoryController::handleGetById(const std::string& id, Response& response) {
    auto dto = service_->getById(id);  // Returns optional<InventoryItemDto>
    if (!dto) {
        sendErrorResponse(response, "Inventory not found", 404);
        return;
    }
    sendJsonResponse(response, dto->toJson().dump());
}

void InventoryController::handleReserve(const std::string& id, Request& request, Response& response) {
    try {
        std::istream& bodyStream = request.stream();
        json body;
        bodyStream >> body;
        
        int quantity = body["quantity"].get<int>();
        auto result = service_->reserve(id, quantity);  // Returns OperationResultDto
        
        sendJsonResponse(response, result.toJson().dump());
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), 400);
    }
}
```

**Controller Best Practices:**
- **Never access models** - Only work with DTOs from services
- Parse URI path and method
- Extract path parameters (e.g., `/api/v1/inventory/:id`)
- Parse query parameters
- Parse JSON request body
- Return JSON responses with proper status codes
- Use try-catch for exception handling

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

### DTO Validation Testing (CRITICAL)

**ALWAYS create comprehensive DTO validation tests** for every service. These tests prevent runtime validation failures that would otherwise cause 500 errors in production.

**Why DTO Tests Are Critical:**
- DTOs validate at construction time (UUID format, enum values, timestamp format)
- Models with invalid data can crash services when converted to DTOs
- Enum string conversions are error-prone without tests
- DtoMapper logic needs verification
- Catch validation issues at build time, not in production

**Required Test File:** `tests/DtoMapperTests.cpp`

**Test Pattern Template:**
```cpp
#include <catch2/catch_all.hpp>
#include "<service>/utils/DtoMapper.hpp"
#include "<service>/models/<Entity>.hpp"
#include "<service>/dtos/<Entity>Dto.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace <service>;

// Helper: Create ISO 8601 timestamp
std::string createIso8601Timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// Helper: Create valid entity model
models::Entity createValidEntity() {
    models::Entity entity("550e8400-e29b-41d4-a716-446655440000", ...);
    entity.setStatus(models::Status::Active);
    entity.setCreatedAt(createIso8601Timestamp());
    entity.setUpdatedAt(createIso8601Timestamp());
    return entity;
}

TEST_CASE("DtoMapper converts valid entity", "[dto][mapper]") {
    auto entity = createValidEntity();
    
    SECTION("Successful conversion with required fields") {
        auto dto = utils::DtoMapper::toEntityDto(entity, "IDENTITY-CODE");
        
        REQUIRE(dto.getId() == entity.getId());
        REQUIRE(dto.getStatus() == "active");
        REQUIRE(dto.getCreatedAt() == entity.getCreatedAt());
    }
    
    SECTION("Conversion with optional fields") {
        entity.setDescription("Test description");
        auto dto = utils::DtoMapper::toEntityDto(entity, "IDENTITY-CODE");
        
        REQUIRE(dto.getDescription().has_value());
        REQUIRE(dto.getDescription().value() == "Test description");
    }
}

TEST_CASE("DtoMapper handles all enum values", "[dto][mapper][enum]") {
    SECTION("Active status") {
        auto entity = createValidEntity();
        entity.setStatus(models::Status::Active);
        auto dto = utils::DtoMapper::toEntityDto(entity, "ID");
        REQUIRE(dto.getStatus() == "active");
    }
    
    SECTION("Inactive status") {
        auto entity = createValidEntity();
        entity.setStatus(models::Status::Inactive);
        auto dto = utils::DtoMapper::toEntityDto(entity, "ID");
        REQUIRE(dto.getStatus() == "inactive");
    }
}

TEST_CASE("DTO constructor validates fields", "[dto][validation]") {
    SECTION("Valid construction succeeds") {
        REQUIRE_NOTHROW(
            dtos::EntityDto(
                "550e8400-e29b-41d4-a716-446655440000",  // Valid UUID
                "ENTITY-001",                             // Valid code
                "active",                                 // Valid status
                createIso8601Timestamp(),                // Valid timestamp
                createIso8601Timestamp()
            )
        );
    }
    
    SECTION("Invalid UUID throws") {
        REQUIRE_THROWS_WITH(
            dtos::EntityDto(
                "not-a-uuid",  // Invalid
                "ENTITY-001",
                "active",
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("valid UUID")
        );
    }
    
    SECTION("Empty identity field throws") {
        REQUIRE_THROWS_WITH(
            dtos::EntityDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "",  // Empty identity field
                "active",
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Invalid enum value throws") {
        REQUIRE_THROWS_WITH(
            dtos::EntityDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ENTITY-001",
                "invalid-status",  // Invalid enum
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("status")
        );
    }
    
    SECTION("Invalid timestamp throws") {
        REQUIRE_THROWS_WITH(
            dtos::EntityDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ENTITY-001",
                "active",
                "not-a-timestamp",  // Invalid
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("ISO 8601")
        );
    }
    
    SECTION("Negative quantity throws") {
        REQUIRE_THROWS_WITH(
            dtos::EntityDto(
                ...params...,
                -1  // Negative quantity
            ),
            Catch::Matchers::ContainsSubstring("non-negative")
        );
    }
}
```

**Required Test Coverage:**

1. **Valid Conversions:**
   - Model → DTO with all required fields
   - Model → DTO with optional fields
   - Identity field inclusion (entity-prefixed fields)

2. **All Enum Values:**
   - Test EVERY enum value for EVERY enum type
   - Verify string conversion accuracy
   - Example: Status (active, inactive, maintenance)
   - Example: Type (standard, express, return)
   - Example: Priority (low, normal, high, urgent)

3. **DTO Constructor Validation:**
   - Invalid UUID format throws
   - Empty required fields throw
   - Empty identity fields throw
   - Invalid enum values throw
   - Invalid timestamp format throws
   - Negative quantities throw (when NonNegativeInteger)
   - Zero/positive quantities accepted

4. **Edge Cases:**
   - Zero quantities (when allowed)
   - Large quantities (stress test)
   - Optional fields omitted (std::nullopt)
   - JSON fields (address, audit, coordinates)

**Test Helpers (always include):**
```cpp
// ISO 8601 timestamp generator
std::string createIso8601Timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// ISO 8601 date generator (for orderDate, etc.)
std::string createIso8601Date() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d");
    return ss.str();
}

// Valid entity factory
models::Entity createValidEntity() {
    models::Entity entity(
        "550e8400-e29b-41d4-a716-446655440000",  // Deterministic UUID
        "ENTITY-001",                            // Valid code
        ...other required fields...
    );
    entity.setStatus(models::Status::Active);
    entity.setCreatedAt(createIso8601Timestamp());
    entity.setUpdatedAt(createIso8601Timestamp());
    return entity;
}
```

**CMakeLists.txt Integration:**
```cmake
# Test sources
set(TEST_SOURCES
    test_main.cpp
    EntityTests.cpp
    HttpIntegrationTests.cpp
    DtoMapperTests.cpp  # REQUIRED
)

# DTO sources needed for tests
set(DTO_SOURCES
    ${CMAKE_SOURCE_DIR}/src/dtos/EntityDto.cpp
    ${CMAKE_SOURCE_DIR}/src/dtos/EntityListDto.cpp
    ${CMAKE_SOURCE_DIR}/src/dtos/ErrorDto.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/DtoMapper.cpp  # REQUIRED
)

add_executable(service-tests ${TEST_SOURCES} ${DTO_SOURCES})
```

**Test Naming Conventions:**
- Use `[dto]` tag for all DTO-related tests
- Use `[dto][mapper]` for DtoMapper conversion tests
- Use `[dto][validation]` for DTO constructor tests
- Use `[dto][enum]` for enum conversion tests
- Use descriptive SECTION names: "Active status", "Invalid UUID throws"

**Running DTO Tests:**
```bash
# Run all DTO tests
./service-tests "[dto]"

# Run only validation tests
./service-tests "[dto][validation]"

# Run only enum tests
./service-tests "[dto][enum]"
```

**Benefits:**
- ❌ **Before tests**: Invalid model data causes 500 errors in production
- ✅ **After tests**: Invalid data caught at compile/test time
- ❌ **Before tests**: Enum typos ('activ' vs 'active') crash services
- ✅ **After tests**: All enum values tested, typos impossible
- ❌ **Before tests**: Timestamp format assumptions break silently
- ✅ **After tests**: ISO 8601 format enforced and validated

**Example Test Counts:**
- inventory-service: 52 test cases
- warehouse-service: 42 test cases
- order-service: 38 test cases
- **Total**: 132 test cases preventing runtime errors
```

### HTTP Integration Testing Pattern (Template)

All C++ services should follow the same HTTP integration testing approach used by the
inventory-service:

- **Test binary**: Include a dedicated `HttpIntegrationTests.cpp` that:
    - Reads `*_HTTP_INTEGRATION`, `*_HTTP_HOST`, `*_HTTP_PORT` from the environment.
    - Skips cleanly when the integration flag is not set.
    - Uses Poco HTTP client (`Poco::Net::HTTPClientSession`) and a small retry loop to wait
        for the service to become healthy.
    - Sends `X-Service-Api-Key` using the `SERVICE_API_KEY` environment variable when set.
    - Exercises the full HTTP API: health, swagger, list/filter endpoints, and core
        business operations (create/update/stock ops/delete) end-to-end.

- **Service-under-test inside tests container**:
    - The Docker entrypoint should detect when it is running the test binary (e.g.
        `./<service>-tests`) and, when `*_HTTP_INTEGRATION=1` is set, start the HTTP service
        binary in the background inside the same container, bound to `0.0.0.0:<PORT>`.
    - Tests should target `localhost` via `*_HTTP_HOST=localhost` and `*_HTTP_PORT=<PORT>`
        to avoid cross-container DNS and sequencing issues.

- **Auth and configuration**:
    - Implement service-to-service auth using a shared API key:
        - `SERVICE_API_KEY` environment variable takes precedence.
        - `auth.serviceApiKey` in `config/application.json` is the fallback.
    - Controllers use `utils::Auth::authorizeServiceRequest` to enforce:
        - `X-Service-Api-Key: <key>` or `Authorization: ApiKey <key>`.
        - `401` for missing token, `403` for invalid token.
    - HTTP integration tests must set `SERVICE_API_KEY` in the tests container and send
        that key on all authenticated endpoints.

- **Env hygiene in tests**:
    - When tests need to mutate env vars (e.g. `AuthTests`), use an RAII helper that saves
        the previous value in the constructor and restores/unsets it in the destructor so
        subsequent tests (including HTTP tests) still see the container-level configuration.

- **Repository + HTTP test coexistence**:
    - DB-backed repository tests should:
        - Use deterministic fixture IDs (product/warehouse/location/inventory IDs).
        - Explicitly clean up any rows for their test product IDs before and after each
            test section (DELETE by `product_id` and by known `id`s) so HTTP tests or prior
            runs cannot affect aggregate assertions.
    - HTTP tests should:
        - Use their own dedicated UUIDs that do not overlap with repository fixtures.
        - Follow the same domain invariants as services (e.g. zero reserved/allocated
            quantity before DELETE when the service enforces such business rules).

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

## Contract System

### Overview

The warehouse management system uses a comprehensive contract system to ensure consistency across services. See `/contracts/docs/overview.md` for full specification.

**Key Concepts:**
- **Contracts**: Global definitions (Types, Entities, Services) that guide validation
- **Claims**: Service promises to fulfill/reference contracts via DTOs, Requests, Events, Endpoints
- **Versioning**: All contracts are versioned; breaking by default
- **Validation**: Build-time and runtime validation ensures compliance
- **Boundary**: Contracts define inputs/outputs only. Internal DB/model field names may differ as long as requests and DTOs are mapped correctly to the contract. If internal fields differ, treat them as derived/aliased in claims when practical.

### Contract Types

**Entity Contracts** (`/contracts/entities/v1/`):
- Define global entities (Product, Inventory, Warehouse, etc.)
- Specify fields with classifications: Identity, Core, Complete
- Identity fields: Must be shared across systems
- Core fields: Required for all instances
- Complete fields: Optional but must match type if present

**Type Contracts** (`/contracts/types/v1/`):
- Define reusable types (UUID, Email, DateTime, etc.)
- Built on base primitives (string, number, boolean, symbol)
- Include constraints (format, size, pattern, enum, encryption)

### Service Claims

**Location**: Each service maintains `claims.json` in its root directory

**Purpose**: 
- Declare which contracts the service implements
- List supported contract versions
- Specify field-level details (method, security, status)

**Example Structure**:
```json
{
  "service": "inventory-service",
  "version": "1.0.0",
  "fulfilments": [
    {
      "contract": "Inventory",
      "versions": ["1.0"],
      "status": "fulfilled",
      "fields": [
        {
          "name": "id",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        }
      ]
    }
  ],
  "references": [
    {
      "contract": "Product",
      "versions": ["1.0"],
      "requiredFields": ["id", "sku"],
      "optionalFields": ["name"]
    }
  ]
}
```

### Service-Specific Contract Definitions

**Location**: `/services/<lang>/<service>/contracts/`

**Structure**:
```
/services/cpp/inventory-service/
  claims.json                       # Claims manifest
  /contracts/
    /dtos/                          # Service DTOs
      InventoryItemDto.json
      InventoryListDto.json
    /requests/                      # Request definitions
      ReserveInventoryRequest.json
      AllocateInventoryRequest.json
    /events/                        # Event definitions
      InventoryReserved.json
      InventoryAllocated.json
    /endpoints/                     # Endpoint definitions
      GetInventory.json
      ListInventory.json
      ReserveInventory.json
```

### DTOs (Data Transfer Objects)

**Purpose**: Define API response formats and message payloads

**Requirements**:
- Must have `basis` field referencing fulfilments/references
- Entity-prefixed fields (e.g., `ProductId`, `WarehouseName`) for entity data
- Unprefixed fields for computed/service-specific data
- Must include ALL identity fields from referenced entities

**Example**:
```json
{
  "name": "InventoryItemDto",
  "version": "1.0",
  "basis": [
    {"entity": "Inventory", "type": "fulfilment"},
    {"entity": "Product", "type": "reference"}
  ],
  "fields": [
    {"name": "id", "type": "UUID", "required": true, "source": "Inventory.id"},
    {"name": "ProductId", "type": "UUID", "required": true, "source": "Product.id"},
    {"name": "ProductSku", "type": "string", "required": true, "source": "Product.sku"},
    {"name": "quantity", "type": "PositiveInteger", "required": true, "source": "Inventory.quantity"},
    {"name": "availableQuantity", "type": "integer", "required": true, "source": "computed"}
  ]
}
```

### Requests (Commands and Queries)

**Purpose**: Define API input parameters

**Requirements**:
- Commands must specify `type` (Create/Update/Delete/Process)
- Must have `basis` listing affected entities
- Must declare `resultType` (a Dto)
- Parameters use contractual types

**Example**:
```json
{
  "name": "ReserveInventoryRequest",
  "version": "1.0",
  "type": "command",
  "commandType": "Process",
  "basis": ["Inventory"],
  "resultType": "InventoryOperationResultDto",
  "parameters": [
    {"name": "quantity", "type": "PositiveInteger", "required": true},
    {"name": "orderId", "type": "UUID", "required": true}
  ]
}
```

### Events

**Purpose**: Define domain/integration messages

**Requirements**:
- Must specify event type (Create/Update/Delete/Notify)
- Must reference a Dto for data payload
- Must include standard metadata (eventId, timestamp, correlationId, source)

**Example**:
```json
{
  "name": "InventoryReserved",
  "version": "1.0",
  "type": "Notify",
  "dataDto": "InventoryOperationResultDto",
  "description": "Published when inventory is successfully reserved"
}
```

### Endpoints

**Purpose**: Define API endpoints

**Requirements**:
- Must specify URI path with parameters
- Must declare HTTP method
- Must list parameters with locations (Route/Query/Body/Header)
- Must specify result type and error responses
- Optional: reference Service Contract operation

**Example**:
```json
{
  "name": "ReserveInventory",
  "uri": "/api/v1/inventory/{id}/reserve",
  "method": "POST",
  "parameters": [
    {"name": "id", "location": "Route", "type": "UUID", "required": true},
    {"name": "request", "location": "Body", "type": "ReserveInventoryRequest", "required": true}
  ],
  "responses": [
    {"status": 200, "type": "InventoryOperationResultDto"},
    {"status": 404, "type": "ErrorDto"},
    {"status": 409, "type": "ErrorDto"}
  ]
}
```

### Critical Validation Rules

**1. Field Exposure (Fulfilments Only)**:
- Every fulfilled entity field must either:
  - Appear in at least ONE service Dto, OR
  - Be marked `"access": "private"` in claims manifest
- Validated at CI build time
- Example: If Inventory contract has 10 fields, service must expose 9 in DTOs + 1 marked private = 10 total

**2. Identity Fields (References)**:
- All identity fields from referenced entities must be included in DTOs
- Example: If referencing Product, must include `ProductId` and `ProductSku` if both are identity fields

**3. Request Basis Enforcement**:
- Requests can only modify entities declared in their `basis`
- Validated to prevent unintended side effects
- Example: ReserveInventory with basis=[Inventory] cannot modify Product

**4. Naming Conventions**:
- Entity-sourced fields: `<Entity><Field>` (ProductId, WarehouseName)
- Computed fields: No prefix (availableQuantity, totalPrice)
- Must be enforced in DTO definitions

### When to Update Contracts

**Adding New Entity**:
1. Define entity contract in `/contracts/entities/v1/<Entity>.json`
2. Add fulfilment claim to service's `claims.json`
3. Create DTOs in service's `/contracts/dtos/`
4. Ensure all entity fields exposed in DTOs or marked private
5. Update models to implement `toJson()`/`fromJson()`

**Adding New Endpoint**:
1. Define Request in service's `/contracts/requests/`
2. Define result Dto in service's `/contracts/dtos/`
3. Define Endpoint in service's `/contracts/endpoints/`
4. Ensure Request `basis` matches service's fulfilments/references
5. Implement controller and add to OpenAPI

**Adding New Field to Entity**:
1. Update entity contract (new version if breaking)
2. Update service claims.json with field status
3. Add field to at least one service Dto (or mark private)
4. Update model class with getter/setter
5. Update repository queries
6. Update `toJson()`/`fromJson()` methods

**Referencing Another Service's Entity**:
1. Add reference claim to `claims.json`
2. Specify required identity fields
3. Update DTOs to include entity-prefixed identity fields
4. Do NOT need to expose all fields (only identity required)

**Publishing Events**:
1. Define Event in service's `/contracts/events/`
2. Ensure event's data Dto exists
3. Include standard metadata fields
4. Publish after successful state changes
5. Use correlation IDs for tracing

### Contract Validation Workflow

**Pre-commit**:
- Validate contract JSON syntax
- Validate type references exist
- Run local contract linter (if available)

**CI Build**:
- Validate service claims against entity contracts
- Verify all fulfilled fields exposed or marked private
- Verify DTO basis matches claims
- Verify Request basis entities are fulfilled/referenced
- Verify naming conventions (entity-prefixed fields)
- Generate validation report

**Integration Tests**:
- Validate actual API responses match DTOs
- Test field presence and types
- Verify identity fields included for references

### Contract Checklist for New Services

1. **Create directory structure**: `/services/<lang>/<service>/contracts/{dtos,requests,events,endpoints}`
2. **Create claims.json**: Declare fulfilments and references
3. **Define DTOs**: For all API responses and event payloads
4. **Define Requests**: For all API inputs (commands/queries)
5. **Define Events**: For all published messages
6. **Define Endpoints**: For all HTTP endpoints
7. **Validate field exposure**: All fulfilled fields in DTOs or marked private
8. **Validate identity fields**: All referenced entity identity fields included
9. **Validate naming**: Entity-prefixed fields follow convention
10. **Update models**: Implement matching C++ classes with `toJson()`/`fromJson()`

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
2. Create contracts directory structure (/contracts/dtos/, /contracts/requests/, /contracts/events/, /contracts/endpoints/)
3. Create claims.json manifest (declare fulfilments and references)
4. Set up CMakeLists.txt
5. Create Dockerfile and docker-compose.yml
6. Initialize Sqitch (sqitch.conf, sqitch.plan)
7. Create initial migration
8. Create models matching entity contracts (with toJson/fromJson) - **models remain internal**
9. **Create DTO directory structure** (include/{service}/dtos/, src/dtos/)
10. **Define DTO classes for all API responses:**
    - ErrorDto (standard error)
    - EntityItemDto (single entity with references)
    - EntityListDto (paginated lists)
    - OperationResultDto (operation results)
11. **Implement DTOs** (immutable, validated, with toJson())
12. **Create DtoMapper utility** (utils/DtoMapper for model → DTO conversion)
13. Define contract DTOs in /contracts/dtos/
14. Define Requests for all API inputs (in /contracts/requests/)
15. Define Events for all messages (in /contracts/events/)
16. Define Endpoints for all HTTP endpoints (in /contracts/endpoints/)
17. Validate field exposure (all fulfilled fields in DTOs or marked private)
18. Implement repositories (returns models, not DTOs)
19. **Implement services with DTO returns** (convert models to DTOs via DtoMapper)
20. **Implement controllers** (work exclusively with DTOs, never touch models)
21. Add DTO source files to CMakeLists.txt
22. Implement SwaggerGenerator utility
23. Add `/api/swagger.json` endpoint
24. **CRITICAL: Create tests/DtoMapperTests.cpp** (comprehensive DTO validation tests)
25. **Add test helpers** (createIso8601Timestamp, createValidEntity)
26. **Test all enum conversions** (every enum value for every enum type)
27. **Test DTO constructor validation** (UUID, empty fields, invalid enums, timestamps, quantities)
28. **Test model→DTO conversions** (required fields, optional fields, identity fields)
29. **Update tests/CMakeLists.txt** (add DtoMapperTests.cpp + DTO sources + DtoMapper.cpp)
30. Create HTTP integration tests (verify API returns DTOs)
31. Update README.md with API endpoints
32. Create PROJECT_STRUCTURE.md

**New Endpoint Checklist:**
1. Define Request in service's `/contracts/requests/`
2. Define result Dto in service's `/contracts/dtos/` (if not exists)
3. **Create C++ DTO class** (header + implementation with validation)
4. Define Endpoint in service's `/contracts/endpoints/`
5. Ensure Request `basis` matches service's fulfilments/references
6. **Implement service method** (returns DTO, not model)
7. **Implement controller method** (calls service, gets DTO, returns JSON)
8. **Add DTO tests to DtoMapperTests.cpp** (if new DTO created)
9. **Test DTO constructor validation** (all validation rules)
10. **Test DtoMapper conversion** (model → new DTO)
11. Add endpoint to OpenAPI/Swagger
12. Add HTTP integration test
13. Update README.md

**Adding Field to Fulfilled Entity:**
1. Update entity contract in `/contracts/entities/v1/` (new version if breaking)
2. Update service's `claims.json` with field status and security
3. **Add field to at least one DTO class** (or mark private in claims.json)
4. Update model class with getter/setter
5. Update repository queries
6. Update `toJson()`/`fromJson()` methods in model
7. **Update DtoMapper** to include new field in DTO conversion
8. Update database migration if needed

**Referencing New Entity:**
1. Add reference claim to service's `claims.json`
2. Specify required identity fields and optional fields
3. **Update DTOs to include entity-prefixed identity fields** (e.g., ProductId, ProductSku)
4. **Update DtoMapper signature** to accept identity fields as parameters
5. Update models if caching referenced entity data
6. Update relevant Requests if passing referenced entity data
7. Add TODOs for fetching real reference data

**Publishing New Event:**
1. Define Event in service's `/contracts/events/`
2. Ensure event's data Dto exists in `/contracts/dtos/`
3. **Create C++ DTO class for event payload**
4. Include standard metadata fields (eventId, timestamp, correlationId, source)
5. Implement event publishing after successful state changes
6. Use correlation IDs for tracing
7. Update README.md with event documentation

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
- **DTO Architecture Pattern**: `/docs/dto-architecture-pattern.md` (REQUIRED READING for all service development)
- Contract System: `/contracts/docs/overview.md` (REQUIRED READING)
- Contracts Directory: `/contracts/README.md`

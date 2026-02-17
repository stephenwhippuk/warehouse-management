# GitHub Copilot Instructions - Warehouse Management System

## Project Overview

This is a microservices-based warehouse management system with:
- **C++ Services**: High-performance backend services (C++20)
- **C# Services**: Business logic and integration services (.NET 8)
- **TypeScript/Vue.js**: Frontend applications
- **PostgreSQL**: Primary database
- **Redis**: Caching layer
- **Sqitch**: Database migration management

**Shared C++ Frameworks** (REQUIRED for all C++ services):
- **HTTP Framework**: Controller-based routing, middleware pipeline, dependency injection (`/services/cpp/shared/http-framework/`)
- **Messaging Framework**: RabbitMQ event publishing/consuming with production-ready resilience (`/services/cpp/shared/warehouse-messaging/`)

**IMPORTANT**: This project uses a comprehensive **Contract System** to ensure consistency across services. All services must declare contracts (fulfilments/references), define DTOs/Requests/Events/Endpoints, and maintain a `claims.json` manifest. See "Contract System" section below and `/contracts/docs/overview.md` for full details.

**CRITICAL ARCHITECTURE POLICY**: Services MUST return DTOs, NOT domain models. Domain models remain internal to the service/repository layers. DTOs are the external API contract. See "Data Transfer Objects (DTOs)" section and `/docs/patterns/dto-architecture-pattern.md` for complete implementation guide.

## Documentation Organization

**IMPORTANT**: Keep the repository clean during iteration by organizing documentation properly. All generated reports, status documents, and iteration logs MUST go in appropriate folders, NOT at the repository root.

**Documentation Structure** (maintain this organization):
```
docs/
├── api.md                           # API reference (core - no move)
├── contributing.md                  # Contributing guidelines (core - no move)
├── architecture/
│   └── design.md                   # System architecture & design decisions
├── patterns/
│   ├── dto-architecture-pattern.md
│   ├── dto-implementation-checklist.md
│   └── dto-implementation-summary.md
├── database/
│   └── database-scaling-consistency.md
├── deployment/
│   └── index.md                    # Deployment guides
├── services/cpp/
│   ├── migrations.md               # Database migrations
│   ├── framework-migration.md       # HTTP framework migration status
│   ├── cpp-database-migrations.md   # C++ migration patterns
│   ├── cpp-swagger-openapi.md       # Swagger/OpenAPI documentation
│   └── shared-libraries/
│       └── shared-messaging-library-design.md
└── reports/                         # Iteration reports, test results, validations
    ├── PHASE_2_COMPLETION.md
    ├── PHASE_3_COMPLETION.md
    ├── DI_SYSTEM_VALIDATION_REPORT.md
    ├── DOCKER_BUILD_RESULTS.md
    ├── MESSAGE_FLOW_TEST_RESULTS.md
    ├── PRODUCT_SERVICE_REVIEW.md
    └── SERVICE_COMPARISON.md
```

**Rules for Documentation**:
1. **Core docs** (api.md, contributing.md): Stay at repo root reference point
2. **Iteration reports**: Always go to `docs/reports/` (e.g., test results, build reports, completion reports)
3. **Architecture/Design**: Goes to `docs/architecture/` or `docs/patterns/`
4. **Service-specific docs**: Goes to `docs/services/{language}/`
5. **Validation/Test results**: Always into `docs/reports/`
6. **Never create at root level**: All .md files except README.md and LICENSE should be in docs/

**No root-level markdown files** during iteration - keep the repository root clean with only essential project files (README.md, LICENSE, package.json, docker-compose.yml, etc.)

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

**CRITICAL PATTERN**: Repositories MUST use the Database singleton via dependency injection.

**Repository Pattern (REQUIRED for all services):**
```cpp
class InventoryRepository {
public:
    // Constructor accepts IServiceProvider for dependency injection
    explicit InventoryRepository(http::IServiceProvider& provider);
    
    // CRUD operations
    std::optional<models::Inventory> findById(const std::string& id);
    std::vector<models::Inventory> findAll();
    models::Inventory create(const models::Inventory& inventory);
    models::Inventory update(const models::Inventory& inventory);
    bool deleteById(const std::string& id);
    
private:
    // Database singleton (NOT raw pqxx::connection)
    std::shared_ptr<utils::Database> db_;
};

// Implementation
InventoryRepository::InventoryRepository(http::IServiceProvider& provider)
    : db_(provider.getService<utils::Database>()) {
}

// Database queries use db_->getConnection()
std::optional<models::Inventory> InventoryRepository::findById(const std::string& id) {
    pqxx::work txn(*db_->getConnection());  // Get connection from Database singleton
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

### Database Singleton Pattern (MANDATORY)

**WHY Database Singleton**: 
- Central connection management for a specific database
- Thread-safe shared state across requests
- Ready for connection pooling and caching layers
- Proper resource cleanup on shutdown
- Consistent architecture across all services

**Database Class Structure (REQUIRED for all services):**

```cpp
// include/{service}/utils/Database.hpp
namespace {service}::utils {

class Database {
public:
    struct Config {
        std::string host = "localhost";
        int port = 5432;
        std::string database = "{service}_db";
        std::string user = "{service}";
        std::string password;
        int maxConnections = 10;
    };
    
    explicit Database(const Config& config);
    ~Database();
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    // Transaction support
    std::unique_ptr<pqxx::work> beginTransaction();
    
    // Query execution
    pqxx::result execute(const std::string& query);
    pqxx::result executeParams(const std::string& query, const std::vector<std::string>& params);
    
    // Prepared statement support
    void prepare(const std::string& name, const std::string& query);
    pqxx::result executePrepared(const std::string& name, const std::vector<std::string>& params = {});
    
    // Connection pool (simple implementation)
    std::shared_ptr<pqxx::connection> getConnection();
    void releaseConnection(std::shared_ptr<pqxx::connection> conn);

private:
    Config config_;
    std::string connectionString_;
    std::shared_ptr<pqxx::connection> connection_;  // Use shared_ptr, not unique_ptr
    
    std::string buildConnectionString() const;
};

} // namespace
```

**Database Implementation Pattern:**

```cpp
// src/utils/Database.cpp
#include "{service}/utils/Database.hpp"
#include "{service}/utils/Logger.hpp"
#include <sstream>

namespace {service}::utils {

Database::Database(const Config& config)
    : config_(config)
    , connectionString_(buildConnectionString()) {
}

Database::~Database() {
    disconnect();
}

std::string Database::buildConnectionString() const {
    std::ostringstream oss;
    oss << "host=" << config_.host
        << " port=" << config_.port
        << " dbname=" << config_.database
        << " user=" << config_.user
        << " password=" << config_.password;
    return oss.str();
}

bool Database::connect() {
    try {
        connection_ = std::make_shared<pqxx::connection>(connectionString_);
        Logger::info("Database connected successfully");
        return true;
    } catch (const pqxx::broken_connection& e) {
        Logger::error("Database connection failed: {}", e.what());
        return false;
    }
}

void Database::disconnect() {
    if (connection_ && connection_->is_open()) {
        connection_->close();
        Logger::info("Database disconnected");
    }
}

bool Database::isConnected() const {
    return connection_ && connection_->is_open();
}

std::unique_ptr<pqxx::work> Database::beginTransaction() {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    return std::make_unique<pqxx::work>(*connection_);
}

pqxx::result Database::execute(const std::string& query) {
    auto txn = beginTransaction();
    auto result = txn->exec(query);
    txn->commit();
    return result;
}

pqxx::result Database::executeParams(const std::string& query, const std::vector<std::string>& params) {
    (void)params;
    // TODO: Implement parameterized query execution
    auto txn = beginTransaction();
    auto result = txn->exec(query); // Temporary: doesn't use params
    txn->commit();
    return result;
}

void Database::prepare(const std::string& name, const std::string& query) {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    connection_->prepare(name, query);
}

pqxx::result Database::executePrepared(const std::string& name, const std::vector<std::string>& params) {
    (void)params;
    // TODO: Implement prepared statement execution with params
    auto txn = beginTransaction();
    auto result = txn->exec(name);  // Stub implementation
    txn->commit();
    return result;
}

std::shared_ptr<pqxx::connection> Database::getConnection() {
    if (!isConnected()) {
        connect();
    }
    return connection_;
}

void Database::releaseConnection(std::shared_ptr<pqxx::connection> conn) {
    (void)conn;
    // TODO: Implement connection pool return logic
}

} // namespace
```

**Database Registration in Application.cpp (REQUIRED):**

```cpp
void Application::initialize() {
    // ... Logger initialization ...
    
    http::ServiceCollection services;
    
    // =========================================================================
    // CRITICAL: Register Database as Singleton
    // =========================================================================
    auto dbConnStr = dbConnectionString_;  // From environment or config
    services.addService<utils::Database>(
        [dbConnStr](http::IServiceProvider& /* provider */) -> std::shared_ptr<utils::Database> {
            utils::Logger::info("Creating Database singleton");
            
            // Parse connection string to Config (TODO: Make robust parser)
            utils::Database::Config dbConfig;
            size_t slashPos = dbConnStr.rfind('/');
            if (slashPos != std::string::npos) {
                dbConfig.database = dbConnStr.substr(slashPos + 1);
            }
            // TODO: Parse host, port, user, password
            
            auto db = std::make_shared<utils::Database>(dbConfig);
            if (!db->connect()) {
                throw std::runtime_error("Failed to connect to database");
            }
            return db;
        },
        http::ServiceLifetime::Singleton
    );
    
    // Register repositories and services (they will receive Database via DI)
    services.addScoped<repositories::InventoryRepository, repositories::InventoryRepository>();
    services.addScoped<services::IInventoryService, services::InventoryService>();
    
    // ... rest of initialization ...
}

void Application::shutdown() {
    // Database cleanup handled automatically by DI container
    // DO NOT call Database::disconnect() manually
}
```

**Database Queries:**
- Always use parameterized queries (prevent SQL injection)
- Use transactions for multi-statement operations
- Handle exceptions and convert to domain errors
- Access connection via `db_->getConnection()`

**Repository Header Pattern:**
```cpp
#include "{service}/utils/Database.hpp"
#include <http-framework/IServiceProvider.hpp>

class ProductRepository {
public:
    explicit ProductRepository(http::IServiceProvider& provider);
    
private:
    std::shared_ptr<utils::Database> db_;  // Database singleton, NOT pqxx::connection
};
````

### Services

**CRITICAL PATTERN**: Services MUST use interface-based design with dependency injection.

**Service Interface Pattern (REQUIRED for all services):**

```cpp
// include/{service}/services/IInventoryService.hpp
class IInventoryService {
public:
    virtual ~IInventoryService() = default;
    
    // Business operations - return DTOs, not models
    virtual std::optional<dtos::InventoryItemDto> getById(const std::string& id) = 0;
    virtual std::vector<dtos::InventoryItemDto> getAll() = 0;
    virtual dtos::InventoryItemDto create(const models::Inventory& inventory) = 0;
    virtual dtos::InventoryItemDto update(const models::Inventory& inventory) = 0;
    
    // Stock operations - return operation result DTOs
    virtual dtos::InventoryOperationResultDto reserve(const std::string& id, int quantity) = 0;
    virtual dtos::InventoryOperationResultDto release(const std::string& id, int quantity) = 0;
    
    // Validation
    virtual bool isValidInventory(const models::Inventory& inventory) const = 0;
};
```

**Service Implementation Pattern (REQUIRED for all services):**

```cpp
// include/{service}/services/InventoryService.hpp
#include "{service}/services/IInventoryService.hpp"
#include "{service}/repositories/InventoryRepository.hpp"
#include <http-framework/IServiceProvider.hpp>

// CRITICAL: Service MUST inherit from its interface
class InventoryService : public IInventoryService {
public:
    // CRITICAL: Constructor MUST accept IServiceProvider for dependency injection
    explicit InventoryService(http::IServiceProvider& provider);
    
    // Override all interface methods
    std::optional<dtos::InventoryItemDto> getById(const std::string& id) override;
    std::vector<dtos::InventoryItemDto> getAll() override;
    dtos::InventoryItemDto create(const models::Inventory& inventory) override;
    dtos::InventoryItemDto update(const models::Inventory& inventory) override;
    dtos::InventoryOperationResultDto reserve(const std::string& id, int quantity) override;
    dtos::InventoryOperationResultDto release(const std::string& id, int quantity) override;
    bool isValidInventory(const models::Inventory& inventory) const override;
    
private:
    std::shared_ptr<repositories::InventoryRepository> repository_;
    std::shared_ptr<warehouse::messaging::EventPublisher> eventPublisher_;
};

// src/services/InventoryService.cpp
InventoryService::InventoryService(http::IServiceProvider& provider)
    : repository_(provider.getService<repositories::InventoryRepository>())
    , eventPublisher_(provider.getService<warehouse::messaging::EventPublisher>()) {
    // Dependencies resolved automatically from DI container
}
```

**Why This Pattern Is Required:**

1. **Interface Inheritance**: DI container uses `std::is_base_of` to verify implementation inheritance
2. **Constructor Injection**: Services resolve dependencies via `IServiceProvider`, not direct injection
3. **No Try-Catch in Constructors**: Let DI container handle missing dependencies
4. **Override Keywords**: Ensures methods match interface signature

**Common Mistakes (AVOID):**

```cpp
// ❌ WRONG: No interface inheritance
class ProductService {
    explicit ProductService(std::shared_ptr<ProductRepository> repo);
};

// ❌ WRONG: Direct dependency injection
class ProductService : public IProductService {
    explicit ProductService(std::shared_ptr<ProductRepository> repo);
};

// ❌ WRONG: Missing override keywords
class ProductService : public IProductService {
    std::optional<ProductDto> getById(const std::string& id);  // No override
};

// ✅ CORRECT: Interface inheritance + IServiceProvider constructor
class ProductService : public IProductService {
    explicit ProductService(http::IServiceProvider& provider);
    std::optional<ProductDto> getById(const std::string& id) override;
};
```

**Service Registration in Application.cpp:**

```cpp
void Application::initialize() {
    http::ServiceCollection services;
    
    // Register repositories (Scoped - per request)
    services.addScoped<repositories::InventoryRepository, repositories::InventoryRepository>();
    
    // CRITICAL: Register service with interface and implementation types
    services.addScoped<services::IInventoryService, services::InventoryService>();
    
    // Build service provider
    serviceProvider_ = services.buildServiceProvider();
}
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
4. **Add shared frameworks to CMakeLists.txt**:
   - Add `http-framework` subdirectory and link
   - Add `warehouse-messaging` subdirectory and link
5. Create Dockerfile and docker-compose.yml
6. Initialize Sqitch (sqitch.conf, sqitch.plan)
7. Create initial migration
8. Create models matching entity contracts (with toJson/fromJson) - **models remain internal**
9. **Create service interfaces** (IInventoryService, IInventoryRepository, etc.) for DI
10. **Implement services with constructor injection** using `http::IServiceProvider&`
11. **Create DTO directory structure** (include/{service}/dtos/, src/dtos/)
12. **Define DTO classes for all API responses:**
    - ErrorDto (standard error)
    - EntityItemDto (single entity with references)
    - EntityListDto (paginated lists)
    - OperationResultDto (operation results)
13. **Implement DTOs** (immutable, validated, with toJson())
14. **Create DtoMapper utility** (utils/DtoMapper for model → DTO conversion)
15. Define contract DTOs in /contracts/dtos/
16. Define Requests for all API inputs (in /contracts/requests/)
17. Define Events for all messages (in /contracts/events/)
18. Define Endpoints for all HTTP endpoints (in /contracts/endpoints/)
19. Validate field exposure (all fulfilled fields in DTOs or marked private)
20. Implement repositories (returns models, not DTOs)
21. **Implement services with DTO returns** (convert models to DTOs via DtoMapper)
22. **Create controllers using http::ControllerBase** (register routes with Get/Post/Put/Delete)
23. **Implement controller handlers** (return std::string, work exclusively with DTOs)
24. **Configure DI container** in Application.cpp:
    - Register singletons (Database, Logger, Cache)
    - Register scoped services (Repositories, Services)
    - Add ServiceScopeMiddleware FIRST
25. **Initialize messaging**:
    - Create EventPublisher for service
    - Create EventConsumer with routing keys
    - Register event handlers
26. **Set up HttpHost** with middleware and controllers
27. Add DTO source files to CMakeLists.txt
28. Implement SwaggerGenerator utility
29. Add `/api/swagger.json` endpoint
30. **CRITICAL: Create tests/DtoMapperTests.cpp** (comprehensive DTO validation tests)
31. **Add test helpers** (createIso8601Timestamp, createValidEntity)
32. **Test all enum conversions** (every enum value for every enum type)
33. **Test DTO constructor validation** (UUID, empty fields, invalid enums, timestamps, quantities)
34. **Test model→DTO conversions** (required fields, optional fields, identity fields)
35. **Update tests/CMakeLists.txt** (add DtoMapperTests.cpp + DTO sources + DtoMapper.cpp)
36. Create HTTP integration tests (verify API returns DTOs)
37. **Test event publishing/consuming** (with mock publisher in unit tests)
38. Update README.md with API endpoints
39. Create PROJECT_STRUCTURE.md

**New Endpoint Checklist:**
1. Define Request in service's `/contracts/requests/`
2. Define result Dto in service's `/contracts/dtos/` (if not exists)
3. **Create C++ DTO class** (header + implementation with validation)
4. Define Endpoint in service's `/contracts/endpoints/`
5. Ensure Request `basis` matches service's fulfilments/references
6. **Implement service method** (returns DTO, not model)
7. **Add route to controller** using Get/Post/Put/Delete with lambda
8. **Implement handler method** (calls service, gets DTO, returns JSON string)
9. **Add DTO tests to DtoMapperTests.cpp** (if new DTO created)
10. **Test DTO constructor validation** (all validation rules)
11. **Test DtoMapper conversion** (model → new DTO)
12. Add endpoint to OpenAPI/Swagger
13. Add HTTP integration test
14. Update README.md

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
5. **Inject EventPublisher into service** via constructor or DI
6. **Publish event AFTER successful state change** (after DB update)
7. Use warehouse::messaging::Event with routing key format: `domain.action`
8. Use correlation IDs for tracing
9. **Add unit test with mock publisher** to verify event is published
10. Update README.md with event documentation

**New Migration Checklist:**
1. Add with Sqitch: `sqitch add NNN_name -n "Description"`
2. Write deploy script (forward changes)
3. Write revert script (rollback changes)
4. Write verify script (test changes exist)
5. Test locally: deploy → verify → revert → deploy
6. Commit to Git with plan file

## HTTP Framework (Shared Library)

### Overview

All C++ services MUST use the **shared HTTP framework** (`/services/cpp/shared/http-framework/`) for consistent API behavior. The framework provides:

- **Controller-based routing** with automatic parameter extraction
- **Middleware pipeline** for cross-cutting concerns (auth, logging, CORS)
- **Dependency Injection (DI)** container with service lifetimes
- **Plugin system** for extensibility
- **Reduced boilerplate**: 10-20 lines vs 200+ lines of manual routing

**Location**: `/services/cpp/shared/http-framework/`  
**Documentation**: 
- `/services/cpp/shared/http-framework/README.md`
- `/services/cpp/shared/http-framework/MIGRATION_GUIDE.md`
- `/services/cpp/shared/http-framework/examples/di_server.cpp` (complete example)

### Quick Start: Controller Pattern

**Standard Controller Structure**:

```cpp
#include "http-framework/ControllerBase.hpp"
#include "http-framework/HttpContext.hpp"

class InventoryController : public http::ControllerBase {
public:
    explicit InventoryController(std::shared_ptr<InventoryService> service)
        : http::ControllerBase("/api/v1/inventory")
        , service_(service) {
        
        // Register endpoints with fluent API
        Get("/", [this](http::HttpContext& ctx) {
            return this->getAll(ctx);
        });
        
        Get("/{id:uuid}", [this](http::HttpContext& ctx) {
            return this->getById(ctx);
        });
        
        Post("/", [this](http::HttpContext& ctx) {
            return this->create(ctx);
        });
        
        Put("/{id:uuid}", [this](http::HttpContext& ctx) {
            return this->update(ctx);
        });
        
        Delete("/{id:uuid}", [this](http::HttpContext& ctx) {
            return this->deleteById(ctx);
        });
        
        Post("/{id:uuid}/reserve", [this](http::HttpContext& ctx) {
            return this->reserve(ctx);
        });
    }

private:
    std::shared_ptr<InventoryService> service_;
    
    std::string getAll(http::HttpContext& ctx) {
        // Query parameters
        int page = ctx.queryParams.getInt("page").value_or(1);
        int pageSize = ctx.queryParams.getInt("pageSize").value_or(20);
        
        auto items = service_->getAll(page, pageSize);
        json j = json::array();
        for (const auto& item : items) {
            j.push_back(item.toJson());
        }
        return j.dump();
    }
    
    std::string getById(http::HttpContext& ctx) {
        // Route parameters automatically extracted
        std::string id = ctx.routeParams["id"];
        auto item = service_->getById(id);
        
        if (!item) {
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            return R"({"error": "Item not found"})";
        }
        
        return item->toJson().dump();
    }
    
    std::string create(http::HttpContext& ctx) {
        // Parse JSON body
        json body = ctx.getBodyAsJson();
        std::string productId = body["productId"];
        int quantity = body["quantity"];
        
        auto item = service_->create(productId, quantity);
        
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
        return item->toJson().dump();
    }
    
    std::string reserve(http::HttpContext& ctx) {
        std::string id = ctx.routeParams["id"];
        json body = ctx.getBodyAsJson();
        int quantity = body["quantity"];
        
        auto result = service_->reserve(id, quantity);
        if (!result.success) {
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);
            return json{{"error", result.message}}.dump();
        }
        
        return result.toJson().dump();
    }
};
```

### HttpContext API

**Route Parameters** (from URL path):
```cpp
std::string id = ctx.routeParams["id"];
```

**Query Parameters** (from URL query string):
```cpp
// With default value
int page = ctx.queryParams.getInt("page").value_or(1);
std::string filter = ctx.queryParams.get("filter", "all");

// Check existence
if (ctx.queryParams.has("includeArchived")) { /* ... */ }
```

**Request Body**:
```cpp
// Parse JSON body
json body = ctx.getBodyAsJson();

// Or get raw string
std::string raw = ctx.getBodyAsString();
```

**Headers**:
```cpp
std::string authHeader = ctx.getHeader("Authorization", "");
bool hasAuth = ctx.hasHeader("X-Api-Key");
ctx.setHeader("X-Request-Id", requestId);
```

**Response Status**:
```cpp
ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);        // 201
ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);      // 404
ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);       // 409
ctx.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR); // 500
```

**Return Value**: All handlers MUST return `std::string` (JSON serialized response)

### Route Constraints

Use constraints for type validation:

```cpp
Get("/{id:uuid}", ...);     // UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
Get("/{id:int}", ...);      // Integer: \d+
Post("/{id:alpha}", ...);   // Alphabetic: [a-zA-Z]+
```

### Middleware Pattern

**Standard Middleware Structure**:

```cpp
#include "http-framework/Middleware.hpp"

class LoggingMiddleware : public http::Middleware {
public:
    void process(http::HttpContext& ctx, std::function<void()> next) override {
        // Before request
        spdlog::info("→ {} {}", ctx.request.getMethod(), ctx.request.getURI());
        auto start = std::chrono::steady_clock::now();
        
        // Call next middleware in pipeline
        next();
        
        // After request
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        spdlog::info("← {} {}ms", ctx.response.getStatus(), duration.count());
    }
};
```

**Common Middleware Examples**:
- **LoggingMiddleware**: Request/response logging
- **AuthenticationMiddleware**: JWT/API key validation
- **CorsMiddleware**: CORS headers
- **ErrorHandlingMiddleware**: Catch exceptions and return proper error responses (REQUIRED)
- **ServiceScopeMiddleware**: Per-request DI scoping (see DI section)

**CRITICAL: Middleware Ordering**:

The order in which middleware is registered matters. Always follow this pattern:

```cpp
http::HttpHost host(8080, "0.0.0.0");

// 1. ServiceScopeMiddleware MUST be first (enables per-request DI scoping)
host.use(std::make_shared<http::ServiceScopeMiddleware>(provider));

// 2. ErrorHandlingMiddleware SHOULD be second (catches all downstream exceptions)
host.use(std::make_shared<http::ErrorHandlingMiddleware>());

// 3. Other middleware follows
host.use(std::make_shared<LoggingMiddleware>());
host.use(std::make_shared<AuthenticationMiddleware>());
host.use(std::make_shared<CorsMiddleware>());
```

**Why Order Matters**:
- **ServiceScopeMiddleware first**: Creates per-request scope so controllers can resolve scoped services
- **ErrorHandlingMiddleware second**: Wraps all other middleware/controllers to catch exceptions
- If ErrorHandlingMiddleware is last, earlier middleware exceptions won't be caught

**ErrorHandlingMiddleware Pattern**:

```cpp
class ErrorHandlingMiddleware : public http::Middleware {
public:
    void process(http::HttpContext& ctx, std::function<void()> next) override {
        try {
            next(); // Call next middleware/controller
        } catch (const std::invalid_argument& e) {
            sendErrorResponse(ctx, e.what(), 400);
        } catch (const std::runtime_error& e) {
            sendErrorResponse(ctx, e.what(), 500);
        } catch (const std::exception& e) {
            sendErrorResponse(ctx, "Internal server error", 500);
            spdlog::error("Unhandled exception: {}", e.what());
        }
    }
    
private:
    void sendErrorResponse(http::HttpContext& ctx, const std::string& message, int status) {
        ctx.setStatus(status);
        json errorJson = {
            {"error", message},
            {"timestamp", getCurrentIsoTimestamp()},
            {"path", ctx.request.getURI()}
        };
        ctx.setHeader("Content-Type", "application/json");
        std::ostream& out = ctx.response.send();
        out << errorJson.dump();
    }
};
```

**Controller Simplification**:

With ErrorHandlingMiddleware, controllers don't need try-catch blocks:

```cpp
// ❌ OLD WAY: Manual error handling in every handler
std::string handler(http::HttpContext& ctx) {
    try {
        auto service = ctx.getService<IInventoryService>();
        auto result = service->reserve(id, quantity);
        return result.toJson().dump();
    } catch (const std::exception& e) {
        ctx.setStatus(500);
        return json{{"error", e.what()}}.dump();
    }
}

// ✅ NEW WAY: Just throw, ErrorHandlingMiddleware catches
std::string handler(http::HttpContext& ctx) {
    auto service = ctx.getService<IInventoryService>();
    auto result = service->reserve(id, quantity);  // Throws on error
    return result.toJson().dump();                 // ErrorHandlingMiddleware handles exceptions
}
```

### Application Setup (Manual DI)

**Basic setup without DI container**:

```cpp
#include "http-framework/HttpHost.hpp"
#include "controllers/InventoryController.hpp"

int main() {
    // Create services manually
    auto db = std::make_shared<Database>(/* config */);
    auto repository = std::make_shared<InventoryRepository>(db);
    auto service = std::make_shared<InventoryService>(repository);
    
    // Create HTTP host
    http::HttpHost host(8080, "0.0.0.0");
    
    // Add middleware (order matters!)
    host.use(std::make_shared<LoggingMiddleware>());
    host.use(std::make_shared<AuthenticationMiddleware>());
    host.use(std::make_shared<CorsMiddleware>());
    
    // Register controllers
    host.addController(std::make_shared<InventoryController>(service));
    
    // Start server
    spdlog::info("Starting server on port 8080");
    host.start();
    
    return 0;
}
```

### CMakeLists.txt Integration

```cmake
# Add http-framework subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/../shared/http-framework http-framework)

# Link against http-framework
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        http-framework
        # ... other dependencies
)
```

## Dependency Injection (DI) System

### Overview

The HTTP framework includes a **production-ready DI container** for managing service lifecycles and dependencies. Use DI for ALL new services.

**Benefits**:
- Automatic dependency resolution
- Testable architecture (mockable services)
- Clear lifecycle management (Singleton, Scoped, Transient)
- Reduced boilerplate (no manual instantiation)

**Documentation**:
- `/services/cpp/shared/http-framework/MIGRATION_GUIDE.md` (Part 2: DI Migration)
- `/services/cpp/shared/http-framework/examples/di_server.cpp` (complete working example)
- `/services/cpp/shared/http-framework/ORIGINAL_PHASE_5_COMPLETE.md` (validation results)

### Service Lifetimes

**Singleton** (created once, shared across entire application):
- Use for: Database connections, connection pools, caches, loggers
- Example: `IDatabase`, `ILogger`, `ICache`
- Thread-safe, shared state

**Scoped** (created per HTTP request, disposed after response):
- Use for: Repositories, business services, per-request context
- Example: `IInventoryRepository`, `IInventoryService`
- Isolated per request, no shared state between requests

**Transient** (created every time requested):
- Use for: Lightweight, stateless operations
- Example: Validators, mappers
- New instance each time

### Quick Start: DI-Enabled Service

#### Step 1: Define Service Interface

```cpp
// include/{service}/services/IInventoryService.hpp
class IInventoryService {
public:
    virtual ~IInventoryService() = default;
    virtual std::vector<InventoryItem> getAll() const = 0;
    virtual std::optional<InventoryItem> getById(const std::string& id) const = 0;
    virtual InventoryItem create(const std::string& productId, int quantity) = 0;
    virtual bool reserve(const std::string& id, int quantity) = 0;
};
```

#### Step 2: Implement with Constructor Injection

```cpp
// src/services/InventoryService.cpp
#include "http-framework/ServiceProvider.hpp"

class InventoryService : public IInventoryService {
public:
    // Constructor takes IServiceProvider and resolves dependencies
    explicit InventoryService(http::IServiceProvider& provider)
        : repository_(provider.getService<IInventoryRepository>())
        , logger_(provider.getService<ILogger>()) {
        logger_->info("InventoryService created (Scoped)");
    }
    
    ~InventoryService() override {
        logger_->info("InventoryService destroyed (Scoped - request complete)");
    }
    
    std::vector<InventoryItem> getAll() const override {
        return repository_->findAll();
    }
    
    std::optional<InventoryItem> getById(const std::string& id) const override {
        return repository_->findById(id);
    }
    
private:
    std::shared_ptr<IInventoryRepository> repository_;
    std::shared_ptr<ILogger> logger_;
};
```

#### Step 3: Register Services in Application

```cpp
#include "http-framework/ServiceCollection.hpp"
#include "http-framework/ServiceProvider.hpp"
#include "http-framework/ServiceScopeMiddleware.hpp"

int main() {
    // =========================================================================
    // Step 1: Configure DI Container
    // =========================================================================
    http::ServiceCollection services;
    
    // Infrastructure (Singleton - shared across app)
    services.addService<IDatabase, Database>(http::ServiceLifetime::Singleton);
    services.addService<ILogger, Logger>(http::ServiceLifetime::Singleton);
    services.addService<ICache, RedisCache>(http::ServiceLifetime::Singleton);
    
    // Data Access (Scoped - per request)
    services.addService<IInventoryRepository, InventoryRepository>(
        http::ServiceLifetime::Scoped
    );
    
    // Business Logic (Scoped - per request)
    services.addService<IInventoryService, InventoryService>(
        http::ServiceLifetime::Scoped
    );
    
    // Build service provider
    auto provider = services.buildServiceProvider();
    
    // =========================================================================
    // Step 2: Configure HTTP Host
    // =========================================================================
    http::HttpHost host(8080, "0.0.0.0");
    
    // CRITICAL: Add ServiceScopeMiddleware FIRST to enable per-request scoping
    host.use(std::make_shared<http::ServiceScopeMiddleware>(provider));
    
    // Other middleware
    host.use(std::make_shared<LoggingMiddleware>());
    host.use(std::make_shared<AuthenticationMiddleware>());
    
    // =========================================================================
    // Step 3: Register Controllers (can now use DI)
    // =========================================================================
    host.addController(std::make_shared<InventoryController>(*provider));
    
    // =========================================================================
    // Step 4: Start Server
    // =========================================================================
    spdlog::info("Server running at http://localhost:8080");
    host.start();
    
    return 0;
}
```

#### Step 4: Controllers Resolve Services from Request Scope

```cpp
class InventoryController : public http::ControllerBase {
public:
    // Controller stores reference to root provider
    explicit InventoryController(http::IServiceProvider& provider)
        : http::ControllerBase("/api/v1/inventory")
        , provider_(provider) {
        
        Get("/", [this](http::HttpContext& ctx) {
            return this->getAll(ctx);
        });
    }

private:
    http::IServiceProvider& provider_;
    
    std::string getAll(http::HttpContext& ctx) {
        // Resolve scoped service from REQUEST SCOPE (via HttpContext)
        // ServiceScopeMiddleware creates a new scope for each request
        auto service = ctx.getService<IInventoryService>();
        
        // Service is created fresh for this request
        // Dependencies (repository, logger) also created in this scope
        auto items = service->getAll();
        
        // After response, scope is destroyed and all scoped services cleaned up
        json j = json::array();
        for (const auto& item : items) {
            j.push_back(item.toJson());
        }
        return j.dump();
    }
};
```

### DI Service Resolution Pattern

**From HttpContext** (recommended for handlers):
```cpp
std::string handler(http::HttpContext& ctx) {
    // Resolve service from request scope
    auto service = ctx.getService<IInventoryService>();
    
    // Use service (automatically destroyed after response)
    auto result = service->doWork();
    return result.toJson().dump();
}
```

**From IServiceProvider** (for constructors):
```cpp
class MyService {
public:
    explicit MyService(http::IServiceProvider& provider)
        : dependency_(provider.getService<IDependency>()) {
    }
private:
    std::shared_ptr<IDependency> dependency_;
};
```

### Service Lifecycle Example

**Singleton Lifecycle** (created once at startup):
```
App Start:
  [Database] ✅ Created (Singleton)
  [Logger] ✅ Created (Singleton)

Request 1:
  (singletons reused)

Request 2:
  (singletons reused)

App Shutdown:
  [Logger] 🗑️ Destroyed
  [Database] 🗑️ Destroyed
```

**Scoped Lifecycle** (created per request):
```
Request 1:
  [ServiceScope] ✅ Created for request
  [InventoryRepository] ✅ Created (Scoped) - uses Singleton Database
  [InventoryService] ✅ Created (Scoped) - uses Repository
  → Handler executes
  [InventoryService] 🗑️ Destroyed (Scoped)
  [InventoryRepository] 🗑️ Destroyed (Scoped)
  [ServiceScope] 🗑️ Destroyed

Request 2:
  [ServiceScope] ✅ Created for request
  [InventoryRepository] ✅ Created (Scoped) - NEW instance
  [InventoryService] ✅ Created (Scoped) - NEW instance
  → Handler executes
  [InventoryService] 🗑️ Destroyed (Scoped)
  [InventoryRepository] 🗑️ Destroyed (Scoped)
  [ServiceScope] 🗑️ Destroyed
```

### Testing with DI

**Mock services for unit tests**:

```cpp
#include <catch2/catch_all.hpp>
#include "http-framework/ServiceCollection.hpp"

// Mock implementation
class MockInventoryRepository : public IInventoryRepository {
public:
    MOCK_METHOD(std::optional<InventoryItem>, findById, (const std::string&), (override));
    MOCK_METHOD(std::vector<InventoryItem>, findAll, (), (override));
};

TEST_CASE("InventoryService with mocked dependencies", "[service][di]") {
    // Setup DI container with mocks
    http::ServiceCollection services;
    services.addService<IInventoryRepository, MockInventoryRepository>(
        http::ServiceLifetime::Scoped
    );
    services.addService<IInventoryService, InventoryService>(
        http::ServiceLifetime::Scoped
    );
    
    auto provider = services.buildServiceProvider();
    auto scope = provider->createScope();
    
    // Get service to test
    auto service = scope->getServiceProvider().getService<IInventoryService>();
    
    // Test behavior
    REQUIRE(service != nullptr);
    auto items = service->getAll();
    REQUIRE(items.empty()); // Mock returns empty
}
```

### Common DI Patterns

**Pattern 1: Service depends on other services**:
```cpp
class InventoryService {
    explicit InventoryService(http::IServiceProvider& provider)
        : repository_(provider.getService<IInventoryRepository>())
        , logger_(provider.getService<ILogger>())
        , cache_(provider.getService<ICache>()) {
    }
};
```

**Pattern 2: Service depends on configuration**:
```cpp
class Database {
    explicit Database(http::IServiceProvider& provider) {
        // Get config from environment or config service
        std::string connectionString = std::getenv("DATABASE_URL");
        connect(connectionString);
    }
};
```

**Pattern 3: Factory pattern**:
```cpp
services.addService<IInventoryRepository>(
    http::ServiceLifetime::Scoped,
    [](http::IServiceProvider& provider) {
        auto db = provider.getService<IDatabase>();
        auto logger = provider.getService<ILogger>();
        return std::make_shared<InventoryRepository>(db, logger);
    }
);
```

### DI Best Practices

1. **Always use interfaces** - Services should depend on abstractions (`IService`), not concrete implementations
2. **Use constructor injection** - Dependencies passed via constructor, stored as members
3. **Choose correct lifetime**:
   - Singleton: Shared resources (DB, cache, logger)
   - Scoped: Request-specific (repositories, services)
   - Transient: Stateless utilities (validators)
4. **ServiceScopeMiddleware first** - Must be first middleware to enable per-request scoping
5. **Resolve from HttpContext** - In handlers, use `ctx.getService<T>()` for request scope
6. **No circular dependencies** - If Service A needs Service B needs Service A, refactor
7. **Test with mocks** - Mock interfaces for unit testing

## Messaging Framework (RabbitMQ)

### Overview

All C++ services MUST use the **shared messaging library** (`/services/cpp/shared/warehouse-messaging/`) for event-driven communication. The library provides:

- **Simple API**: 10-20 lines vs 200+ lines of RabbitMQ boilerplate
- **Production-ready resilience**: Durable queues, manual ACK, auto-retry, DLQ
- **Automatic reconnection**: Survives broker restarts
- **Thread-safe**: Safe for concurrent use

**Location**: `/services/cpp/shared/warehouse-messaging/`  
**Documentation**: `/services/cpp/shared/warehouse-messaging/README.md`

### Quick Start: Publishing Events

```cpp
#include "warehouse/messaging/EventPublisher.hpp"
#include "warehouse/messaging/Event.hpp"

using namespace warehouse::messaging;

int main() {
    // Create publisher (uses environment variables for config)
    auto publisher = EventPublisher::create("inventory-service");
    
    // Create event
    json data = {
        {"id", "550e8400-e29b-41d4-a716-446655440000"},
        {"productId", "prod-123"},
        {"quantity", 50},
        {"status", "reserved"}
    };
    
    Event event("inventory.reserved", data, "inventory-service");
    
    // Publish (fire-and-forget)
    publisher->publish(event);
    
    spdlog::info("Published inventory.reserved event");
    
    return 0;
}
```

### Quick Start: Consuming Events

```cpp
#include "warehouse/messaging/EventConsumer.hpp"
#include "warehouse/messaging/Event.hpp"

using namespace warehouse::messaging;

int main() {
    // Create consumer
    std::vector<std::string> routingKeys = {
        "inventory.reserved",
        "inventory.released",
        "inventory.allocated"
    };
    
    auto consumer = EventConsumer::create("order-service", routingKeys);
    
    // Register event handlers
    consumer->onEvent("inventory.reserved", [](const Event& event) {
        std::string id = event.getData()["id"];
        int quantity = event.getData()["quantity"];
        spdlog::info("Inventory reserved: {} units of {}", quantity, id);
        
        // Process event (update order status, etc.)
        // If this throws, event is automatically retried (max 3 times)
        // After retries exhaused, message moves to DLQ
    });
    
    consumer->onEvent("inventory.released", [](const Event& event) {
        spdlog::info("Inventory released: {}", event.getData()["id"].dump());
        // Handle release...
    });
    
    // Start consuming (blocking or background thread)
    consumer->start();
    
    // Keep app running...
    std::this_thread::sleep_for(std::chrono::hours(24));
    
    // Graceful shutdown
    consumer->stop();
    
    return 0;
}
```

### Event Structure

**Standard Event Fields**:
```cpp
Event event(
    "domain.action",           // Routing key (e.g., "product.created")
    jsonData,                  // Event payload (JSON)
    "source-service"           // Source service name
);

// Automatically includes:
event.getEventId();          // UUID
event.getTimestamp();        // ISO 8601 timestamp
event.getCorrelationId();    // Optional correlation ID
event.getRoutingKey();       // Routing key
event.getData();             // JSON payload
event.getSource();           // Source service
```

### Configuration

**Environment Variables** (production-ready defaults):

```bash
# RabbitMQ Connection
export RABBITMQ_HOST=localhost
export RABBITMQ_PORT=5672
export RABBITMQ_USER=warehouse
export RABBITMQ_PASSWORD=warehouse_dev
export RABBITMQ_VHOST=/
export RABBITMQ_EXCHANGE=warehouse.events

# Service Identity
export SERVICE_NAME=inventory-service
```

**Embedded Resilience (No Config Needed)**:
- ✅ Durable queues (survive broker restart)
- ✅ Persistent messages (survive broker crash)
- ✅ Manual ACK (reliable processing)
- ✅ Auto-retry (3 attempts with exponential backoff)
- ✅ Dead Letter Queue (failed messages)
- ✅ Auto-reconnect (recover from connection loss)
- ✅ QoS Prefetch (load balancing)

### Integration with Services

**In Application.cpp**:

```cpp
#include "warehouse/messaging/EventPublisher.hpp"
#include "warehouse/messaging/EventConsumer.hpp"

class Application {
public:
    void initialize() {
        // Initialize publisher
        publisher_ = EventPublisher::create("inventory-service");
        
        // Initialize consumer
        std::vector<std::string> routingKeys = {"product.created", "product.updated"};
        consumer_ = EventConsumer::create("inventory-service", routingKeys);
        
        // Register handlers
        consumer_->onEvent("product.created", [this](const Event& event) {
            handleProductCreated(event);
        });
        
        // Start consuming in background
        consumer_->start();
        
        // Start HTTP server
        startHttpServer();
    }
    
    void shutdown() {
        consumer_->stop();
        // Clean shutdown
    }
    
private:
    std::shared_ptr<EventPublisher> publisher_;
    std::shared_ptr<EventConsumer> consumer_;
    
    void handleProductCreated(const Event& event) {
        // Business logic here
        // Throws exception on failure → automatic retry
    }
};
```

### Publishing Events from Services

**Pattern: Publish after successful state change**:

```cpp
class InventoryService {
public:
    InventoryService(
        std::shared_ptr<IInventoryRepository> repository,
        std::shared_ptr<EventPublisher> publisher)
        : repository_(repository)
        , publisher_(publisher) {
    }
    
    bool reserve(const std::string& id, int quantity) {
        // 1. Update database
        auto inventory = repository_->findById(id);
        if (!inventory || inventory->getAvailableQuantity() < quantity) {
            return false;
        }
        
        inventory->reserve(quantity);
        repository_->update(*inventory);
        
        // 2. Publish event (fire-and-forget)
        json eventData = {
            {"id", id},
            {"productId", inventory->getProductId()},
            {"quantity", quantity},
            {"reservedQuantity", inventory->getReservedQuantity()},
            {"availableQuantity", inventory->getAvailableQuantity()}
        };
        
        Event event("inventory.reserved", eventData, "inventory-service");
        publisher_->publish(event);
        
        return true;
    }
    
private:
    std::shared_ptr<IInventoryRepository> repository_;
    std::shared_ptr<EventPublisher> publisher_;
};
```

### Event Handler Best Practices

1. **Idempotent handlers** - Handle duplicate messages gracefully (check if already processed)
2. **Fast processing** - Keep handlers lightweight (< 1 second)
3. **Error handling** - Throw exceptions to trigger automatic retry
4. **Dead letter monitoring** - Monitor DLQ for persistent failures
5. **Correlation IDs** - Use for request tracing across services

### Messaging Patterns

**Pattern 1: Domain Events** (notify other services of state changes):
```cpp
// Routing keys: domain.action
"inventory.reserved"
"inventory.released"
"product.created"
"order.placed"
```

**Pattern 2: Integration Events** (cross-service workflows):
```cpp
// order-service publishes
Event("order.placed", {...}, "order-service");

// inventory-service consumes and publishes
Event("inventory.allocated", {...}, "inventory-service");

// warehouse-service consumes
// (order.placed → inventory.allocated → warehouse.pick_requested)
```

**Pattern 3: Event Sourcing** (audit trail):
```cpp
// Store all state changes as events
Event("inventory.quantity_adjusted", {...});
Event("inventory.status_changed", {...});
// Rebuild state by replaying events
```

### CMakeLists.txt Integration

```cmake
# Add warehouse-messaging subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/../shared/warehouse-messaging warehouse-messaging)

# Link against warehouse-messaging
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        warehouse::warehouse-messaging
        # ... other dependencies
)
```

### Testing Messaging

**Mock EventPublisher for unit tests**:

```cpp
class MockEventPublisher : public EventPublisher {
public:
    std::vector<Event> publishedEvents;
    
    void publish(const Event& event) override {
        publishedEvents.push_back(event);
    }
};

TEST_CASE("Service publishes event on reserve", "[service][messaging]") {
    auto mockPublisher = std::make_shared<MockEventPublisher>();
    InventoryService service(repository, mockPublisher);
    
    service->reserve("id-123", 10);
    
    REQUIRE(mockPublisher->publishedEvents.size() == 1);
    REQUIRE(mockPublisher->publishedEvents[0].getRoutingKey() == "inventory.reserved");
}
```

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

## Common Pitfalls and Solutions

### DI and Service Registration

**Problem: Service not inheriting from interface**
```cpp
// ❌ ERROR: DI container cannot verify inheritance
class ProductService {  // Missing `: public IProductService`
    std::optional<ProductDto> getById(const std::string& id);
};

// ✅ SOLUTION: Always inherit from interface
class ProductService : public IProductService {
    std::optional<ProductDto> getById(const std::string& id) override;
};
```

**Problem: Missing override keywords**
```cpp
// ❌ ERROR: Typo in method signature won't be caught
class ProductService : public IProductService {
    std::optional<ProductDto> getByID(const std::string& id);  // Wrong: ID vs Id
};

// ✅ SOLUTION: Use override keyword (compiler error on mismatch)
class ProductService : public IProductService {
    std::optional<ProductDto> getById(const std::string& id) override;
};
```

**Problem: Direct dependency injection instead of IServiceProvider**
```cpp
// ❌ ERROR: Hard-coded dependencies, not DI-friendly
class ProductService : public IProductService {
    explicit ProductService(std::shared_ptr<ProductRepository> repo)
        : repo_(repo) {}
};

// ✅ SOLUTION: Accept IServiceProvider, resolve from container
class ProductService : public IProductService {
    explicit ProductService(http::IServiceProvider& provider)
        : repo_(provider.getService<ProductRepository>()) {}
};
```

**Problem: Registering concrete type instead of interface**
```cpp
// ❌ ERROR: Controllers can't resolve IProductService
services.addScoped<ProductService, ProductService>();

// ✅ SOLUTION: Register with interface type
services.addScoped<IProductService, ProductService>();
```

### Database Singleton Pattern

**Problem: Using static Database methods**
```cpp
// ❌ OLD PATTERN: Static methods, not DI-friendly
class Database {
public:
    static void connect(const std::string& connStr);
    static std::shared_ptr<pqxx::connection> getConnection();
};

// ✅ NEW PATTERN: Instance-based singleton via DI
class Database {
public:
    struct Config { /* ... */ };
    explicit Database(const Config& config);
    bool connect();
    std::shared_ptr<pqxx::connection> getConnection();
private:
    std::shared_ptr<pqxx::connection> connection_;
};
```

**Problem: Repository storing pqxx::connection directly**
```cpp
// ❌ ERROR: Bypasses Database singleton management
class ProductRepository {
    explicit ProductRepository(std::shared_ptr<pqxx::connection> db);
private:
    std::shared_ptr<pqxx::connection> db_;
};

// ✅ SOLUTION: Store Database singleton, call getConnection()
class ProductRepository {
    explicit ProductRepository(http::IServiceProvider& provider);
private:
    std::shared_ptr<utils::Database> db_;  // Database singleton
};

// Usage in queries
pqxx::work txn(*db_->getConnection());
```

**Problem: Manual Database::disconnect() in shutdown**
```cpp
// ❌ ERROR: DI container already manages lifecycle
void Application::shutdown() {
    utils::Database::disconnect();  // Don't do this
}

// ✅ SOLUTION: Let DI container handle cleanup
void Application::shutdown() {
    // Database will be destroyed automatically by DI container
    // No manual cleanup needed
}
```

### Middleware Pipeline

**Problem: Wrong middleware order**
```cpp
// ❌ ERROR: ServiceScopeMiddleware not first, ErrorHandlingMiddleware not second
host.use(std::make_shared<LoggingMiddleware>());
host.use(std::make_shared<ErrorHandlingMiddleware>());
host.use(std::make_shared<ServiceScopeMiddleware>(provider));

// ✅ SOLUTION: Correct ordering
host.use(std::make_shared<ServiceScopeMiddleware>(provider));  // 1. FIRST
host.use(std::make_shared<ErrorHandlingMiddleware>());         // 2. SECOND
host.use(std::make_shared<LoggingMiddleware>());               // 3. Other middleware
```

**Problem: Try-catch in every controller handler**
```cpp
// ❌ ERROR: Duplicated error handling (use ErrorHandlingMiddleware instead)
std::string handler(http::HttpContext& ctx) {
    try {
        auto service = ctx.getService<IProductService>();
        return service->getById(id)->toJson().dump();
    } catch (const std::exception& e) {
        ctx.setStatus(500);
        return json{{"error", e.what()}}.dump();
    }
}

// ✅ SOLUTION: Let ErrorHandlingMiddleware catch exceptions
std::string handler(http::HttpContext& ctx) {
    auto service = ctx.getService<IProductService>();
    auto product = service->getById(id);
    if (!product) {
        throw std::runtime_error("Product not found");  // ErrorHandlingMiddleware catches
    }
    return product->toJson().dump();
}
```

### Build and Compilation

**Problem: Forward declaration not resolved**
```cpp
// ❌ ERROR: Forward declaration insufficient after changing to instance usage
// ProductService.hpp
class ProductRepository;  // Forward declaration

class ProductService {
    std::shared_ptr<ProductRepository> repo_;  // Needs complete type
};

// ✅ SOLUTION: Include full header
#include "product/repositories/ProductRepository.hpp"

class ProductService {
    std::shared_ptr<ProductRepository> repo_;
};
```

**Problem: Unused parameter warnings**
```cpp
// ❌ WARNING: parameter 'params' set but not used
pqxx::result Database::executeParams(const std::string& query, 
                                      const std::vector<std::string>& params) {
    // TODO: Implement
    return execute(query);
}

// ✅ SOLUTION: Cast to void to suppress warning
pqxx::result Database::executeParams(const std::string& query, 
                                      const std::vector<std::string>& params) {
    (void)params;  // Suppress unused warning
    // TODO: Implement parameterized queries
    return execute(query);
}
```

**Problem: Missing include guards breaking compilation**
```cpp
// ❌ ERROR: Multiple definition errors
// Database.hpp
class Database { /* ... */ };

// ✅ SOLUTION: Always use include guards
#ifndef PRODUCT_UTILS_DATABASE_HPP
#define PRODUCT_UTILS_DATABASE_HPP

class Database { /* ... */ };

#endif // PRODUCT_UTILS_DATABASE_HPP
```

### Architecture Consistency

**Key Lesson: All services in the project must use the same patterns**

When you update one service's architecture (e.g., Database singleton), update ALL services:
- ✅ inventory-service: Database singleton ✓
- ✅ product-service: Database singleton ✓
- ✅ warehouse-service: Database singleton ✓
- ❌ order-service: Still using old pattern (TODO)

**Benefits of Consistency**:
- Easier code review (same patterns everywhere)
- Faster onboarding (learn once, apply everywhere)
- Simpler maintenance (fix in one place, know how to fix everywhere)
- Reduced bugs (proven patterns, fewer surprises)

## References

- C++ Core Guidelines: https://isocpp.github.io/CppCoreGuidelines/
- Sqitch Documentation: https://sqitch.org/
- Poco Documentation: https://pocoproject.org/docs/
- Project Architecture: `/docs/architecture/design.md`
- Database Migrations: `/docs/services/cpp/cpp-database-migrations.md`
- **DTO Architecture Pattern**: `/docs/patterns/dto-architecture-pattern.md` (REQUIRED READING for all service development)
- Contract System: `/contracts/docs/overview.md` (REQUIRED READING)
- Contracts Directory: `/contracts/README.md`

### HTTP Framework & Dependency Injection

- **HTTP Framework README**: `/services/cpp/shared/http-framework/README.md`
- **Migration Guide**: `/services/cpp/shared/http-framework/MIGRATION_GUIDE.md` (REQUIRED - comprehensive HTTP + DI migration)
- **DI Example Server**: `/services/cpp/shared/http-framework/examples/di_server.cpp` (complete working example)
- **DI Completion Report**: `/services/cpp/shared/http-framework/ORIGINAL_PHASE_5_COMPLETE.md` (validation & testing results)
- **DI Example Documentation**: `/services/cpp/shared/http-framework/examples/DI_SERVER_EXAMPLE.md` (how to run example)

### Messaging Framework (RabbitMQ)

- **Messaging Library README**: `/services/cpp/shared/warehouse-messaging/README.md` (REQUIRED - production-ready messaging)
- **Publisher Example**: `/services/cpp/shared/warehouse-messaging/examples/simple_publisher.cpp`
- **Consumer Example**: `/services/cpp/shared/warehouse-messaging/examples/simple_consumer.cpp`
- **Message Flow Test Results**: `/MESSAGE_FLOW_TEST_RESULTS.md` (end-to-end validation)


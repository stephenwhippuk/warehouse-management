# Inventory Service - Project Structure

## Directory Tree

```
inventory-service/
├── CMakeLists.txt                   # Main CMake configuration
├── Dockerfile                       # Docker image build (service + tests)
├── docker-compose.yml              # Docker compose with PostgreSQL/Redis/RabbitMQ for app + tests
├── README.md                       # Comprehensive documentation
├── .gitignore                      # Git ignore rules
│
├── config/                         # Configuration files
│   ├── application.json            # Application configuration
│   └── .env.example               # Environment variables template
│
├── include/inventory/             # Public headers
│   ├── Application.hpp            # Main application class
│   ├── Server.hpp                 # HTTP server wrapper + routing helper
│   │
│   ├── models/                    # Domain models
│   │   └── Inventory.hpp          # Inventory entity with operations
│   │
│   ├── controllers/               # HTTP request handlers
│   │   ├── InventoryController.hpp # Inventory endpoints
│   │   ├── HealthController.hpp    # /health endpoint
│   │   └── SwaggerController.hpp   # /api/swagger.json endpoint
│   │
│   ├── repositories/              # Data access layer
│   │   └── InventoryRepository.hpp # Inventory database operations
│   │
│   ├── services/                  # Business logic layer
│   │   └── InventoryService.hpp   # Inventory business logic + event publishing
│   │
│   └── utils/                     # Utility classes
│       ├── Database.hpp           # PostgreSQL connection
│       ├── Logger.hpp             # Logging wrapper (spdlog)
│       ├── Config.hpp             # Configuration management
│       ├── JsonValidator.hpp      # JSON Schema validation
│       ├── MessageBus.hpp         # Abstract message bus interface
│       ├── RabbitMqMessageBus.hpp # RabbitMQ implementation (rabbitmq-c)
│       ├── Auth.hpp               # Service-to-service API key auth helper
│       └── SwaggerGenerator.hpp   # OpenAPI/Swagger spec generation
│
├── src/                           # Implementation files
│   ├── main.cpp                   # Entry point
│   ├── Application.cpp            # Application implementation
│   ├── Server.cpp                 # Server implementation
│   ├── STUBS.md                   # Stub implementation status
│   │
│   ├── models/
│   │   └── Inventory.cpp          # Inventory entity implementation
│   │
│   ├── controllers/
│   │   ├── InventoryController.cpp # Inventory controller
│   │   ├── HealthController.cpp    # Health endpoint implementation
│   │   └── SwaggerController.cpp   # Swagger/OpenAPI controller
│   │
│   ├── repositories/
│   │   └── InventoryRepository.cpp # Inventory repository (stub)
│   │
│   ├── services/
│   │   └── InventoryService.cpp   # Inventory service (complete, publishes events)
│   │
│   └── utils/
│       ├── Database.cpp           # Database implementation (partial)
│       ├── Logger.cpp             # Logger implementation (complete)
│       ├── Config.cpp             # Config implementation (complete)
│       ├── JsonValidator.cpp      # Validator implementation (partial)
│       ├── RabbitMqMessageBus.cpp # RabbitMQ-backed MessageBus implementation
│       ├── Auth.cpp               # Service-to-service auth implementation
│       └── SwaggerGenerator.cpp   # Swagger/OpenAPI helper implementation
│
├── tests/                         # Test files
│   ├── CMakeLists.txt            # Test configuration
│   ├── test_main.cpp             # Catch2 main entry point
│   ├── InventoryTests.cpp        # Inventory model tests
│   ├── InventoryRepositoryTests.cpp # Repository + DB integration tests
│   ├── InventoryServiceBusTests.cpp # Service wiring with MessageBus stub
│   ├── RabbitMqIntegrationTests.cpp # Real RabbitMQ publish integration test
│   ├── AuthTests.cpp             # Service-to-service auth tests
│   └── RoutingTests.cpp          # HTTP routing tests (/health, /api/swagger.json)
│
└── migrations/                    # Database migrations
    └── 001_init.sql              # Initial schema with triggers

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
         ┌─────────▼─────────┐
         │  Controller       │
         │  Inventory        │
         └─────────┬─────────┘
                   │
         ┌─────────▼─────────┐
         │  Service          │  Business Logic:
         │  Inventory        │  - Validation
         │                   │  - Stock operations
         │                   │  - Business rules
         └─────────┬─────────┘
                   │
         ┌─────────▼─────────┐
         │  Repository       │  Data Access:
         │  Inventory        │  - CRUD operations
         │                   │  - Queries
         │                   │  - Aggregations
         └─────────┬─────────┘
                   │
         ┌─────────▼─────────┐
         │   Database.cpp    │
         │   (PostgreSQL)    │
         └───────────────────┘
```

## Key Features

### ✅ Implemented
- **Project structure** with proper separation of concerns
- **Domain model** (Inventory) matching JSON Schema contracts
- **Business operations**: reserve, release, allocate, deallocate, adjust
- **Validation logic** in service layer
- **CMake build system** with dependency management
- **Docker support** with multi-stage builds
- **Configuration management** (JSON + environment variables)
- **Logging** with spdlog integration
- **Database schema** with triggers and constraints
- **HTTP server** scaffolding with Poco
- **Unit tests** framework with Catch2
- **Movement tracking** table for audit trail
 - **Service-to-service auth** via internal API key
 - **Health endpoint** at `/health` exposing basic auth metrics

### 🚧 Stub/Partial Implementation
- JSON Schema validation (structure complete, needs implementation)
- Database connection pooling

### 📝 TODO for Full Implementation
1. **Validation**: Complete JSON Schema validation integration
2. **Connection Pooling**: Implement database connection pool
3. **Integration Tests**: Expand HTTP integration tests into full HTTP API
    end-to-end coverage
4. **Metrics**: Add Prometheus metrics endpoint (expose existing counters)
5. **Low Stock Alerts**: Implement real-time alerting

## Data Model

### Inventory Entity

```cpp
class Inventory {
    // Identifiers
    string id, productId, warehouseId, locationId;
    
    // Quantities
    int quantity;                 // Total on hand
    int availableQuantity;        // Available for reservation
    int reservedQuantity;         // Reserved for orders
    int allocatedQuantity;        // Allocated to picks
    
    // Tracking
    optional<string> serialNumber, batchNumber;
    optional<string> expirationDate, manufactureDate;
    optional<double> costPerUnit;
    
    // Status
    InventoryStatus status;       // available, reserved, damaged, etc.
    QualityStatus qualityStatus;  // passed, failed, pending
    
    // Operations
    void reserve(int qty);
    void release(int qty);
    void allocate(int qty);
    void deallocate(int qty);
    void adjust(int change, string reason);
    bool isExpired();
    bool isLowStock(int threshold);
};
```

### Quantity Relationship

```
quantity = availableQuantity + reservedQuantity + allocatedQuantity
```

Enforced by database trigger.

## API Endpoints (Planned)

### Inventory Operations
- `GET    /api/v1/inventory` - List all inventory
- `GET    /api/v1/inventory/:id` - Get by ID
- `GET    /api/v1/inventory/product/:productId` - Get by product
- `GET    /api/v1/inventory/warehouse/:warehouseId` - Get by warehouse
- `GET    /api/v1/inventory/location/:locationId` - Get by location
- `GET    /api/v1/inventory/low-stock?threshold=N` - Low stock items
- `GET    /api/v1/inventory/expired` - Expired items
- `POST   /api/v1/inventory` - Create inventory record
- `PUT    /api/v1/inventory/:id` - Update inventory
- `DELETE /api/v1/inventory/:id` - Delete inventory

### Stock Operations
- `POST   /api/v1/inventory/:id/reserve` - Reserve quantity
- `POST   /api/v1/inventory/:id/release` - Release reservation
- `POST   /api/v1/inventory/:id/allocate` - Allocate to shipment
- `POST   /api/v1/inventory/:id/deallocate` - Deallocate
- `POST   /api/v1/inventory/:id/adjust` - Adjust quantity

### Aggregate Queries
- `GET    /api/v1/inventory/product/:id/total` - Total quantity
- `GET    /api/v1/inventory/product/:id/available` - Available quantity

## Database Schema

### `inventory` Table
- Tracks stock at each location
- Quantity breakdown (available/reserved/allocated)
- Batch/serial tracking
- Expiry monitoring
- Quality status
- Constraints ensure quantity = available + reserved + allocated

### `inventory_movements` Table
- Audit trail of all changes
- Movement types: receive, issue, transfer, adjust, reserve, etc.
- References to source transactions
- Automatic logging via triggers

### Database Triggers
1. **update_updated_at**: Updates timestamp on modification
2. **validate_inventory_quantities**: Enforces quantity relationship
3. **log_inventory_movement**: Auto-logs quantity changes

## Dependencies

### Required
- **C++20** compiler (GCC 11+, Clang 13+, MSVC 2022+)
- **CMake** 3.20+
- **Boost** (system, thread)
- **Poco** (Net, NetSSL, Util, Foundation)
- **PostgreSQL** client library (libpqxx)
- **nlohmann/json** for JSON parsing
- **spdlog** for logging

### Optional
- **Catch2** for testing
- **Redis++** for caching
- **JSON Schema Validator** for request validation

## Building

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install -y build-essential cmake libboost-all-dev \
    libpoco-dev libpq-dev libpqxx-dev nlohmann-json3-dev libspdlog-dev

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run
./bin/inventory-service config/application.json

# Test
ctest --output-on-failure
```

## Docker

```bash
# Build and run with Docker Compose
docker-compose up -d

# Service available at http://localhost:8081
# PostgreSQL at localhost:5433
# Redis at localhost:6380
```

## Status

**Current State**: ✅ Core functionality implemented (repositories, services, controllers, Swagger, DB + RabbitMQ tests)  
**Next Step**: Implement JSON Schema validation, metrics endpoint, and full HTTP API integration tests  
**Estimated Completion**: 1-2 days for remaining hardening work

See [STUBS.md](src/STUBS.md) for detailed implementation status.

## Differences from Warehouse Service

- **Focus**: Stock quantities vs. physical locations
- **Operations**: Reserve/allocate vs. status changes
- **Tracking**: Movement history with audit trail
- **Alerts**: Low stock and expiry monitoring
- **Relationships**: Many-to-one (product→locations) vs one-to-many (warehouse→locations)

## Integration Points

- **Product Service**: Product information lookup
- **Warehouse Service**: Location validation
- **Order Service**: Reservation and allocation
- **Shipment Service**: Allocation and fulfillment

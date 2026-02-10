# Order Service - Project Structure

## Overview

The order-service is a C++20 microservice for managing orders in the warehouse management system. It follows the same architectural patterns as inventory-service and warehouse-service.

## Directory Structure

```
order-service/
├── claims.json                     # Service contract manifest
├── CMakeLists.txt                  # Root build configuration
├── Dockerfile                      # Multi-stage Docker build
├── docker-compose.yml              # Local development setup
├── docker-entrypoint.sh            # Container startup script
├── sqitch.conf                     # Database migration config
├── sqitch.plan                     # Migration plan (empty - pending)
├── README.md                       # Service documentation
├── PROJECT_STRUCTURE.md            # This file
│
├── config/
│   └── application.json            # Application configuration
│
├── contracts/                      # Service contract definitions
│   ├── dtos/
│   │   ├── ErrorDto.json           # Standard error response
│   │   ├── OrderDto.json           # Order data transfer object
│   │   └── OrderListDto.json       # Paginated order list
│   ├── requests/
│   │   ├── CreateOrderRequest.json # Order creation parameters
│   │   ├── UpdateOrderRequest.json # Order update parameters
│   │   └── CancelOrderRequest.json # Order cancellation parameters
│   ├── events/
│   │   ├── OrderCreated.json       # Order creation event
│   │   ├── OrderUpdated.json       # Order update event
│   │   └── OrderCancelled.json     # Order cancellation event
│   └── endpoints/
│       ├── GetOrderById.json       # GET /api/v1/orders/{id}
│       ├── ListOrders.json         # GET /api/v1/orders
│       ├── CreateOrder.json        # POST /api/v1/orders
│       ├── UpdateOrder.json        # PUT /api/v1/orders/{id}
│       └── CancelOrder.json        # POST /api/v1/orders/{id}/cancel
│
├── include/order/                  # Public headers
│   ├── models/
│   │   └── Order.hpp               # Order model (Order, OrderLineItem, Address)
│   ├── controllers/
│   │   ├── OrderController.hpp     # Order HTTP controller
│   │   ├── HealthController.hpp    # Health endpoint
│   │   └── ClaimsController.hpp    # Contract discovery endpoints
│   ├── services/
│   │   └── OrderService.hpp        # Business logic layer
│   ├── repositories/
│   │   └── OrderRepository.hpp     # Data access layer (stubs)
│   ├── utils/
│   │   ├── Auth.hpp                # API key authentication
│   │   ├── Config.hpp              # Configuration management
│   │   └── Logger.hpp              # Logging utilities
│   └── Server.hpp                  # HTTP server
│
├── src/                            # Implementation files
│   ├── main.cpp                    # Application entry point
│   ├── Server.cpp                  # HTTP server implementation
│   ├── models/
│   │   └── Order.cpp               # Order model implementation
│   ├── controllers/
│   │   ├── OrderController.cpp     # Order controller implementation
│   │   ├── HealthController.cpp    # Health endpoint implementation
│   │   └── ClaimsController.cpp    # Claims endpoint implementation
│   ├── services/
│   │   └── OrderService.cpp        # Business logic (stubs)
│   ├── repositories/
│   │   └── OrderRepository.cpp     # Data access (stubs)
│   └── utils/
│       ├── Auth.cpp                # Authentication implementation
│       ├── Config.cpp              # Configuration implementation
│       └── Logger.cpp              # Logging implementation
│
├── tests/                          # Test files
│   ├── CMakeLists.txt              # Test build configuration
│   └── HttpIntegrationTests.cpp    # HTTP API integration tests
│
├── migrations/                     # Database migrations (pending)
│   ├── deploy/                     # Forward migrations
│   ├── revert/                     # Rollback scripts
│   └── verify/                     # Verification tests
│
└── build/                          # Build artifacts (gitignored)
```

## Key Files

### Configuration & Build

- **CMakeLists.txt**: Root build configuration using C++20, links Poco, nlohmann/json, spdlog
- **config/application.json**: Server config (port 8082), database, logging, auth
- **claims.json**: Contract manifest declaring Order fulfilment, Product/Warehouse references

### Models

- **Order.hpp/cpp**: 
  - `Order` class with status, priority, line items, addresses
  - `OrderLineItem` struct for product references
  - `Address` struct for shipping/billing
  - Enums: `OrderStatus` (11 states), `OrderPriority` (4 levels)
  - JSON serialization (`toJson`/`fromJson`)
  - Business logic (`calculateTotal`, `canBeCancelled`, `cancel`)

### Controllers

- **OrderController**: Main REST API handling GET/POST/PUT/POST (cancel)
- **HealthController**: Service health endpoint
- **ClaimsController**: Contract discovery endpoints (claims, fulfilments, references)

All controllers enforce API key authentication via `Auth::authorizeServiceRequest`.

### Services & Repositories

- **OrderService**: Business logic layer (stub implementations with TODOs)
- **OrderRepository**: Data access layer (stub implementations with TODOs)

Both follow the same pattern as inventory-service and warehouse-service.

### Utilities

- **Auth**: API key authentication (X-Service-Api-Key or Authorization: ApiKey header)
- **Config**: JSON configuration with env var overrides
- **Logger**: spdlog wrapper with structured logging

### HTTP Server

- **Server**: Poco HTTPServer with routing:
  - `/health` → HealthController
  - `/api/v1/claims*` → ClaimsController
  - `/api/v1/orders*` → OrderController

### Tests

- **HttpIntegrationTests.cpp**: Catch2 tests for HTTP API
  - Health endpoint
  - Claims endpoint
  - List orders
  - Authentication
  - CRUD operations (expecting 501 Not Implemented for now)

### Docker

- **Dockerfile**: Multi-stage build (builder + runtime)
- **docker-compose.yml**: PostgreSQL + order-service
- **docker-entrypoint.sh**: Waits for DB, runs migrations, starts service or tests

## Contract System Integration

The service implements the warehouse management contract system:

### Fulfilments

Declares fulfilment of **Order** entity (v1.0) with 7 core fields:
- id (UUID, identity)
- orderNumber (string, identity)
- customerId (string, required)
- warehouseId (UUID, required)
- status (OrderStatus enum)
- orderDate (DateTime)
- total (number)

### References

Declares references to:
- **Product** (v1.0): For line item product data
- **Warehouse** (v1.0): For order warehouse context

### DTOs

All DTOs follow naming conventions:
- Entity-sourced fields: Capitalized (WarehouseId, WarehouseCode, ProductId, ProductSku)
- Computed fields: Lowercase (availableQuantity, lineTotal)
- Must include all identity fields from referenced entities

### Endpoints

All endpoints have ApiKey authentication, proper status codes (200, 201, 400, 401, 404, 500, 501).

### Events

Events include standard metadata:
- eventId (UUID)
- timestamp (DateTime)
- correlationId (UUID, optional)
- source (string, "order-service")

## Development Workflow

1. **Building**: `mkdir build && cd build && cmake .. && cmake --build .`
2. **Running**: `./build/order-service` (or `docker-compose up`)
3. **Testing**: `docker-compose run --rm -e ORDER_HTTP_INTEGRATION=1 -e SERVICE_API_KEY=... order-service ./order-service-tests`
4. **Database**: Sqitch migrations (pending implementation)

## Status

✅ **Complete**: Structure, models, controllers, auth, tests, Docker
⏳ **Pending**: Database schema, repository implementations, service logic, event publishing

## Architecture

Follows clean layered architecture:
```
HTTP Request
    ↓
Controller (validates, parses)
    ↓
Service (business logic)
    ↓
Repository (data access)
    ↓
Database
```

All layers use dependency injection via `std::shared_ptr`.

## Dependencies

- **Poco**: HTTP server, networking
- **nlohmann/json**: JSON parsing
- **spdlog**: Logging
- **Catch2**: Testing
- **PostgreSQL** (libpq, planned)
- **Sqitch** (migrations, planned)

## Next Steps

1. Create initial database schema migration
2. Implement OrderRepository with PostgreSQL
3. Implement OrderService business logic
4. Add input validation
5. Implement event publishing
6. Add Swagger/OpenAPI endpoint
7. Complete HTTP integration tests
8. Add performance testing

## References

- Main README: `README.md`
- Contract System: `/contracts/docs/overview.md`
- Architecture Guide: `/.github/copilot-instructions.md`
- Similar Services: `../inventory-service/`, `../warehouse-service/`

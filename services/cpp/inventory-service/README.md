# Inventory Service

C++20 microservice for managing warehouse inventory, stock levels, and inventory movements.

## Features

- ✅ Inventory tracking with quantity management
- ✅ Multi-location inventory support
- ✅ Reserve/Release/Allocate/Deallocate operations
- ✅ Expiry date tracking and alerts
- ✅ Batch and serial number tracking
- ✅ Quality control status
- ✅ Inventory movement history
- ✅ Low stock detection
- ✅ Real-time stock availability

## Architecture

```
Controllers → Services → Repositories → Database (PostgreSQL)
```

### Layered Design

- **Models**: Domain entities matching JSON Schema contracts
- **Controllers**: HTTP request handlers (Poco HTTPServer)
- **Services**: Business logic and validation
- **Repositories**: Data access layer (PostgreSQL with pqxx)
- **Utils**: Cross-cutting concerns (logging, config, database)

## API Endpoints

### Inventory Management

```
GET    /api/v1/inventory                    - List all inventory
GET    /api/v1/inventory/:id                - Get inventory by ID
GET    /api/v1/inventory/product/:id        - Get inventory by product
GET    /api/v1/inventory/warehouse/:id      - Get inventory by warehouse
GET    /api/v1/inventory/location/:id       - Get inventory by location
GET    /api/v1/inventory/low-stock          - Get low stock items
GET    /api/v1/inventory/expired            - Get expired items
POST   /api/v1/inventory                    - Create inventory record
PUT    /api/v1/inventory/:id                - Update inventory
DELETE /api/v1/inventory/:id                - Delete inventory
```

### Stock Operations

```
POST   /api/v1/inventory/:id/reserve        - Reserve quantity
POST   /api/v1/inventory/:id/release        - Release reserved quantity
POST   /api/v1/inventory/:id/allocate       - Allocate to shipment
POST   /api/v1/inventory/:id/deallocate     - Deallocate from shipment
POST   /api/v1/inventory/:id/adjust         - Adjust quantity (cycle count)
```

### Health & Diagnostics

```
GET    /health                              - Service health and auth metrics
```

The health endpoint returns a simple JSON payload including:
- Overall service status (e.g. `"ok"`)
- Service name
- In-memory counters for auth outcomes (authorized, missingToken, invalidToken)

### API Documentation

```
GET    /api/swagger.json                    - OpenAPI 3.0 specification
```

View the complete API documentation by accessing the Swagger endpoint:
- Development: http://localhost:8081/api/swagger.json
- Docker: http://inventory-service:8081/api/swagger.json

Use with Swagger UI, Postman, or other OpenAPI-compatible tools.

### Query Parameters

```
?threshold=N           - Low stock threshold
?includeExpired=true   - Include expired items
?status=available      - Filter by status
```

## Building

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libpoco-dev \
    libpq-dev \
    libpqxx-dev \
    nlohmann-json3-dev \
    libspdlog-dev
```

### Build Steps

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Run

```bash
./bin/inventory-service config/application.json
```

### Run Tests

```bash
cd build
ctest --output-on-failure
```

To run DB-backed repository tests, point to a test database with the inventory-service Sqitch migrations applied:

```bash
export INVENTORY_TEST_DATABASE_URL="postgresql://user:pass@localhost:5432/inventory_test"
ctest --output-on-failure
```

## Docker

### Build Image

```bash
docker build -t inventory-service:latest .
```

### Run with Docker Compose

```bash
docker-compose up -d
```

Services:
- **inventory-service**: `http://localhost:8081`
- **PostgreSQL**: `localhost:5433`
- **Redis**: `localhost:6380`

### Running Integration Tests with Docker

The inventory-service includes integration tests that exercise:
- Repository operations against a real PostgreSQL database
- Message bus publishing against a real RabbitMQ broker
 - Full HTTP API behaviour against a running inventory-service instance

Use the service-specific docker-compose to run the full test stack:

```bash
cd services/cpp/inventory-service
docker-compose up --build inventory-tests
```

This will start:
- `postgres` (inventory test database)
- `redis` (for future caching features)
- `rabbitmq` (service bus for message bus integration tests)
- `inventory-tests` container, which:
  - builds and runs `./inventory-service-tests`
  - starts `./inventory-service` in the same container when `INVENTORY_HTTP_INTEGRATION=1`
  - runs HTTP integration tests against `http://localhost:8080`

RabbitMQ and DB connection details for tests are configured via environment variables in
`docker-compose.yml` (e.g. `INVENTORY_TEST_DATABASE_URL`, `INVENTORY_RABBITMQ_INTEGRATION`,
`RABBITMQ_HOST`, `RABBITMQ_USER`, etc.). HTTP integration tests are enabled via:

- `INVENTORY_HTTP_INTEGRATION=1`
- `INVENTORY_HTTP_HOST=localhost`
- `INVENTORY_HTTP_PORT=8080`

When these variables are set, the test binary will:
- use a short retry loop when calling `/health` and `/api/swagger.json`
- send `X-Service-Api-Key` using the `SERVICE_API_KEY` environment variable
- exercise create/read/filter/stock operation/delete flows end-to-end over HTTP

## Configuration

### Environment Variables

```bash
DATABASE_URL=postgresql://user:pass@host:port/db
SERVER_PORT=8080
LOG_LEVEL=info
REDIS_URL=redis://localhost:6379
LOW_STOCK_THRESHOLD=10
SERVICE_API_KEY=your_internal_service_key   # Optional: enables service-to-service auth
```

### Configuration File

See `config/application.json` for full configuration options.

Authentication settings:

```json
"auth": {
  "serviceApiKey": "your_internal_service_key"
}
```

When `SERVICE_API_KEY` (env) or `auth.serviceApiKey` (config) is set, the
inventory-service enforces service-to-service authentication:

- Requests must include either:
  - `X-Service-Api-Key: <key>`
  - `Authorization: ApiKey <key>`
- Missing credentials → `401 Unauthorized`
- Invalid credentials → `403 Forbidden`

The `/health` and `/api/swagger.json` endpoints are intentionally left
unauthenticated to support monitoring and API discovery.

## Database Schema

### `inventory` Table

- Tracks current stock levels at each location
- Quantity breakdown: available, reserved, allocated
- Batch/lot and serial number tracking
- Expiry date monitoring
- Quality control status

### `inventory_movements` Table

- Audit trail of all inventory changes
- Movement types: receive, issue, transfer, adjust, etc.
- References to source transactions
- Automatic logging via triggers

## Business Logic

### Quantity Relationships

```
total_quantity = available + reserved + allocated
```

- **Available**: Ready for reservation
- **Reserved**: Allocated to orders but not picked
- **Allocated**: Assigned to picks/shipments

### State Machine

```
AVAILABLE → RESERVED → ALLOCATED → (shipped)
     ↓           ↓           ↓
  QUARANTINE  DAMAGED    EXPIRED
```

## Operations

### Reserve Inventory

Reserves quantity for an order:
```cpp
service->reserve(inventoryId, quantity);
```

### Release Reservation

Cancels a reservation:
```cpp
service->release(inventoryId, quantity);
```

### Allocate to Shipment

Converts reservation to allocation:
```cpp
service->allocate(inventoryId, quantity);
```

### Adjust Quantity

Manual adjustment (cycle count):
```cpp
service->adjust(inventoryId, quantityChange, "Cycle count correction");
```

## JSON Schema Validation

Validates against `/contracts/schemas/v1/inventory.schema.json`:
- Required fields
- Quantity constraints
- Status enums
- Date formats

## Monitoring

### Health Check

```bash
curl http://localhost:8080/health
```

### Metrics (Planned)

A dedicated metrics endpoint (for example, Prometheus-style `/metrics`) is planned but
not yet implemented.

## Development Status

### ✅ Complete

- Project structure
- Domain models with JSON serialization
- Business logic for stock operations
- Database schema with triggers
- Docker containerization
- Unit tests
- Configuration management

### 🚧 TODO

- JSON Schema validation wired to request handling
- Metrics endpoint exposing existing in-memory counters
- Expand HTTP integration tests into full HTTP API coverage ✅ (implemented in `tests/HttpIntegrationTests.cpp`)
- Concurrent operation tests
- Database connection pooling
- Authentication/authorization

## Database Migrations

Migrations use **Sqitch** for version-controlled schema changes:

```bash
# Check status
sqitch status

# Deploy migrations
sqitch deploy

# Verify migrations
sqitch verify

# Rollback
sqitch revert

# Add new migration
sqitch add 002_add_alerts -n "Add low stock alerts"
```

**Structure:**
- `migrations/deploy/` - Forward migrations
- `migrations/revert/` - Rollback scripts  
- `migrations/verify/` - Verification tests
- `sqitch.conf` - Configuration
- `sqitch.plan` - Migration plan

**Docker:** Migrations run automatically via `docker-entrypoint.sh`

## Testing

```bash
# Build and run tests
cd build
make inventory-service-tests
./bin/inventory-service-tests

# Run HTTP integration tests (service must be running and reachable)
INVENTORY_HTTP_INTEGRATION=1 \
INVENTORY_HTTP_HOST=localhost \
INVENTORY_HTTP_PORT=8080 \
ctest -R http --output-on-failure
./bin/inventory-service-tests "[inventory][operations]"
```

## Dependencies

- **C++20**: Modern C++ features
- **CMake 3.20+**: Build system
- **Boost**: System utilities
- **Poco**: HTTP server framework
- **PostgreSQL (pqxx)**: Database client
- **nlohmann/json**: JSON parsing
- **spdlog**: Logging
- **Catch2**: Testing framework

## Documentation

### Event Consumption Architecture

Complete guides for implementing production-ready message consumption:

1. **[MULTI_SERVICE_EVENT_FLOW.md](docs/MULTI_SERVICE_EVENT_FLOW.md)** - START HERE! 
   - Visual diagrams showing how multiple services consume the same events
   - Real-world example with timeline
   - Answers: "How do inventory-service AND order-service both get product events?"

2. **[EVENT_DISTRIBUTION_PATTERNS.md](docs/EVENT_DISTRIBUTION_PATTERNS.md)**
   - Quick reference guide
   - Fanout vs Competing Consumers patterns
   - Common mistakes and how to avoid them
   - Testing strategies

3. **[EVENT_CONSUMPTION_ARCHITECTURE.md](docs/EVENT_CONSUMPTION_ARCHITECTURE.md)**
   - Complete implementation guide
   - Resilience patterns and error handling
   - Idempotency strategies
   - Production configuration details

4. **[CONSUMER_RESILIENCE_CHECKLIST.md](docs/CONSUMER_RESILIENCE_CHECKLIST.md)**
   - Step-by-step implementation tasks
   - Code snippets with before/after
   - Testing procedures for each phase
   - Completion criteria

5. **[MULTI_SERVICE_CONSUMPTION_SUMMARY.md](docs/MULTI_SERVICE_CONSUMPTION_SUMMARY.md)**
   - Executive summary
   - Implementation status
   - What's correct, what needs fixing

### Key Topics Covered
- **Fanout Pattern**: Multiple services consuming the same events (inventory + order both need product events)
- **Competing Consumers**: Multiple instances load balancing within one service
- **Manual ACK with Retry**: Ensuring no message loss on errors
- **Dead Letter Queues**: Handling permanently failed messages
- **Idempotency**: Safe message reprocessing strategies
- **Horizontal Scaling**: Running multiple service instances

### Quick Reference

**Current Queue Configuration**:
```cpp
queue_name: "inventory-service-products"  // ✅ Unique per service (enables fanout)
routing_keys: ["product.created", "product.updated", "product.deleted"]
```

**Multi-Service Event Flow**:
```
product-service publishes product.created
    ↓
warehouse.events exchange
    ├─> inventory-service-products queue → inventory-service processes
    └─> order-service-products queue → order-service processes
    
Result: Both services receive the SAME event independently ✅
```

**See [MULTI_SERVICE_EVENT_FLOW.md](docs/MULTI_SERVICE_EVENT_FLOW.md) for complete visual diagrams.**

## Contributing

1. Follow C++ Core Guidelines
2. Maintain test coverage
3. Update JSON schemas if needed
4. Document API changes

## License

See root LICENSE file.

## Related Services

- **warehouse-service**: Warehouse and location management
- **order-service**: Order processing
- **product-service**: Product catalog

## Contact

Part of the Warehouse Management System microservices architecture.

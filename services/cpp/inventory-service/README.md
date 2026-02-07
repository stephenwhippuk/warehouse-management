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

Use the service-specific docker-compose to run the full test stack:

```bash
cd services/cpp/inventory-service
docker-compose up --build inventory-tests
```

This will start:
- `postgres` (inventory test database)
- `redis` (for future caching features)
- `rabbitmq` (service bus for message bus integration tests)
- `inventory-tests` container, which builds and runs `./inventory-service-tests`

RabbitMQ and DB connection details for tests are configured via environment variables in
`docker-compose.yml` (e.g. `INVENTORY_TEST_DATABASE_URL`, `INVENTORY_RABBITMQ_INTEGRATION`,
`RABBITMQ_HOST`, `RABBITMQ_USER`, etc.).

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

### Metrics

```bash
curl http://localhost:8080/metrics
```

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

- Database query implementation in repositories
- HTTP routing logic in controllers
- JSON Schema validation
- Metrics and health endpoints
- Integration tests
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

# Run specific test
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

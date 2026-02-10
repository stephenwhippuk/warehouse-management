# Order Service

A high-performance C++ microservice for managing orders in the warehouse management system.

## Overview

The Order Service handles:
- Order creation and lifecycle management
- Order status tracking (pending → confirmed → processing → shipped → delivered)
- Order cancellation with reason tracking
- Line item management
- Integration with Product and Warehouse services via contract references

## Features

- **RESTful API**: HTTP endpoints for CRUD operations
- **Contract-Driven**: Implements Order entity fulfilment, references Product and Warehouse
- **Authentication**: Service-to-service API key authentication
- **Health Monitoring**: `/health` endpoint for service status
- **Contract Discovery**: `/api/v1/claims` endpoints for contract introspection
- **Event Publishing**: Domain events for order state changes (OrderCreated, OrderUpdated, OrderCancelled)

## Technology Stack

- **Language**: C++20
- **HTTP Server**: Poco C++ Libraries
- **JSON**: nlohmann/json
- **Logging**: spdlog
- **Database**: PostgreSQL (planned)
- **Migrations**: Sqitch (planned)
- **Testing**: Catch2
- **Build**: CMake 3.20+
- **Containerization**: Docker

## API Endpoints

### Health & Discovery

- `GET /health` - Service health check
- `GET /api/v1/claims` - Service contract manifest
- `GET /api/v1/claims/fulfilments` - Fulfilled contracts
- `GET /api/v1/claims/references` - Referenced contracts

### Order Management

- `GET /api/v1/orders` - List orders (with filtering)
- `GET /api/v1/orders/{id}` - Get order by ID
- `POST /api/v1/orders` - Create new order
- `PUT /api/v1/orders/{id}` - Update order
- `POST /api/v1/orders/{id}/cancel` - Cancel order

## Configuration

Configuration is read from `config/application.json` and can be overridden with environment variables:

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 8082
  },
  "database": {
    "host": "postgres",
    "database": "order_db",
    "user": "postgres",
    "password": "postgres"
  },
  "auth": {
    "serviceApiKey": "dev-api-key-change-in-production"
  }
}
```

### Environment Variables

- `ORDER_SERVICE_PORT` - HTTP server port (default: 8082)
- `SERVICE_API_KEY` - Service authentication key (overrides config)
- `DATABASE_HOST` - PostgreSQL host
- `DATABASE_NAME` - PostgreSQL database name
- `DATABASE_USER` - PostgreSQL user
- `DATABASE_PASSWORD` - PostgreSQL password

## Building

### Prerequisites

```bash
apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libpoco-dev \
    libpq-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    sqitch \
    libdbd-pg-perl \
    postgresql-client
```

### Build Commands

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)
```

## Running

### Local Development

```bash
# Start service
./build/order-service

# Service runs on http://localhost:8082
```

### Docker

```bash
# Build and run
docker-compose up -d

# View logs
docker-compose logs -f order-service

# Stop services
docker-compose down
```

## Testing

### Unit Tests

```bash
cd build
ctest --output-on-failure
```

### HTTP Integration Tests

HTTP integration tests run inside the Docker container and test the full HTTP API:

```bash
# Build with tests
docker-compose build

# Run tests
docker-compose run --rm \
  -e ORDER_HTTP_INTEGRATION=1 \
  -e ORDER_HTTP_HOST=localhost \
  -e ORDER_HTTP_PORT=8082 \
  -e SERVICE_API_KEY=dev-api-key-change-in-production \
  order-service ./order-service-tests
```

The docker-entrypoint.sh script automatically starts the service in the background when running tests with `ORDER_HTTP_INTEGRATION=1`.

## Contract System

The Order Service implements the contract system for API consistency:

### Fulfilments

- **Order** (v1.0): Core order entity with status tracking

### References

- **Product** (v1.0): References product data for line items
- **Warehouse** (v1.0): References warehouse for order fulfillment

### Service Contracts

All contracts are defined in `contracts/`:

- **DTOs**: `contracts/dtos/` - API response formats
- **Requests**: `contracts/requests/` - API input parameters
- **Events**: `contracts/events/` - Domain event definitions
- **Endpoints**: `contracts/endpoints/` - HTTP endpoint specifications

See `claims.json` for complete contract declarations.

## Development Status

⚠️ **Current Status**: Base structure complete, business logic pending implementation

### Completed
- ✅ Project structure and build system
- ✅ Contract definitions (DTOs, Requests, Events, Endpoints)
- ✅ Model classes (Order, OrderLineItem, Address)
- ✅ Controllers (OrderController, HealthController, ClaimsController)
- ✅ Authentication utilities
- ✅ HTTP integration tests
- ✅ Docker configuration

### TODO
- ⏳ Database schema and migrations
- ⏳ Repository implementations with PostgreSQL
- ⏳ Service business logic (validation, state transitions)
- ⏳ Event publishing (RabbitMQ/Redis)
- ⏳ Swagger/OpenAPI documentation endpoint
- ⏳ Additional HTTP integration tests for full CRUD
- ⏳ Performance testing and optimization

## Architecture

The service follows a layered architecture:

```
Controllers (HTTP) → Services (Business Logic) → Repositories (Data Access) → Database
```

### Key Components

- **Models**: Domain entities (`Order`, `OrderLineItem`, `Address`)
- **Controllers**: HTTP request handlers
- **Services**: Business logic and validation
- **Repositories**: Data access layer (planned)
- **Utils**: Cross-cutting concerns (Auth, Config, Logger)

## Database Schema (Planned)

The service will use PostgreSQL with Sqitch for migrations:

### Tables
- `orders` - Main order records
- `order_line_items` - Order line items (products, quantities, prices)
- `order_status_history` - Audit trail of status changes

## Events (Planned)

The service will publish domain events:

- `OrderCreated` - New order created
- `OrderUpdated` - Order details updated
- `OrderCancelled` - Order cancelled with reason
- `OrderShipped` - Order marked as shipped
- `OrderDelivered` - Order delivered to customer

## Monitoring

- **Health Endpoint**: `/health` returns service status and metrics
- **Logging**: Structured logs via spdlog
- **Metrics**: Timestamp tracking in health endpoint

## Security

- **API Key Authentication**: All endpoints require `X-Service-Api-Key` header
- **Environment Variables**: Sensitive config via env vars
- **Non-root Container**: Runs as user `order` (UID 1000)

## Contributing

Follow the project guidelines in `/docs/contributing.md` and C++ standards in `/.github/copilot-instructions.md`.

## License

See LICENSE file in repository root.

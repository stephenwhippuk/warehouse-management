# Warehouse Service

C++ 20 microservice for warehouse and location management in the warehouse management system.

## Overview

The Warehouse Service handles:
- **Warehouse Management**: CRUD operations for warehouse facilities
- **Location Management**: Storage location tracking and organization
- **Zone Management**: Warehouse zone configuration
- **Route Optimization**: Picking route optimization using location data
- **Capacity Management**: Space utilization and capacity tracking

## Technology Stack

- **Language**: C++ 20
- **HTTP Framework**: Poco (PocoNet)
- **Database**: PostgreSQL (via libpqxx)
- **JSON**: nlohmann/json
- **Logging**: spdlog
- **Validation**: JSON Schema validator
- **Build System**: CMake 3.20+
- **Containerization**: Docker

## Architecture

```
warehouse-service/
├── include/warehouse/      # Public headers
│   ├── models/            # Domain models (Warehouse, Location)
│   ├── controllers/       # HTTP request handlers
│   ├── repositories/      # Data access layer
│   ├── services/          # Business logic layer
│   └── utils/             # Utilities (DB, logging, config)
├── src/                   # Implementation files
│   ├── models/
│   ├── controllers/
│   ├── repositories/
│   ├── services/
│   ├── utils/
│   ├── main.cpp
│   ├── Application.cpp
│   └── Server.cpp
├── tests/                 # Unit and integration tests
├── config/                # Configuration files
├── migrations/            # Database migrations
├── CMakeLists.txt
├── Dockerfile
└── README.md
```

## Prerequisites

### System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libpoco-dev \
    libpq-dev \
    libssl-dev \
    nlohmann-json3-dev \
    libspdlog-dev
```

**macOS:**
```bash
brew install cmake boost poco postgresql nlohmann-json spdlog
```

### Optional Dependencies

- **Redis**: `apt-get install libhiredis-dev redis-plus-plus-dev`
- **JSON Schema Validator**: Build from source or use vcpkg

### Contract Schemas

The service validates requests against JSON Schema contracts in `../../../contracts/schemas/v1/`:
- `warehouse.schema.json`
- `location.schema.json`
- `common.schema.json`

## Building

### Local Build

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --parallel $(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo cmake --install .
```

### Build Options

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Disable tests
cmake .. -DBUILD_TESTS=OFF

# Custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
```

### Docker Build

```bash
# Build image
docker build -t warehouse-service:latest .

# Run container
docker run -p 8080:8080 \
  -e DB_HOST=postgres \
  -e DB_PASSWORD=secret \
  warehouse-service:latest
```

### Docker Compose

```bash
# Start all services (includes PostgreSQL and Redis)
docker-compose up -d

# View logs
docker-compose logs -f warehouse-service

# Stop services
docker-compose down
```

## Configuration

### Configuration File

Create `config/application.json` (see `config/application.json.example`):

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 8080,
    "maxThreads": 10
  },
  "database": {
    "host": "localhost",
    "port": 5432,
    "database": "warehouse_db",
    "user": "warehouse",
    "password": "warehouse_password"
  },
  "logging": {
    "level": "info",
    "file": "logs/warehouse-service.log"
  }
}
```

### Environment Variables

Environment variables override configuration file values:

```bash
WAREHOUSE_HOST=0.0.0.0
WAREHOUSE_PORT=8080
DB_HOST=localhost
DB_PORT=5432
DB_NAME=warehouse_db
DB_USER=warehouse
DB_PASSWORD=secret
LOG_LEVEL=info
```

## Running

### Local

```bash
# Create logs directory
mkdir -p logs

# Run with default config
./build/bin/warehouse-service

# Run with custom config
./build/bin/warehouse-service /path/to/config.json
```

### Docker

```bash
docker run -d \
  --name warehouse-service \
  -p 8080:8080 \
  -v $(pwd)/config:/app/config \
  -v $(pwd)/logs:/app/logs \
  -e DB_HOST=postgres \
  -e DB_PASSWORD=secret \
  warehouse-service:latest
```

## API Endpoints

### API Documentation

```http
GET /api/swagger.json
```

Returns OpenAPI 3.0 specification for the Warehouse Service API. View complete API documentation by accessing:
- Development: http://localhost:8080/api/swagger.json
- Docker: http://warehouse-service:8080/api/swagger.json

Use with Swagger UI, Postman, or other OpenAPI-compatible tools for interactive API testing and client generation.

### Warehouse Endpoints

#### List All Warehouses
```http
GET /api/v1/warehouses
```

#### Get Warehouse by ID
```http
GET /api/v1/warehouses/{id}
```

#### Create Warehouse
```http
POST /api/v1/warehouses
Content-Type: application/json

{
  "code": "WH-001",
  "name": "Main Distribution Center",
  "address": {
    "street": "123 Industrial Pkwy",
    "city": "Chicago",
    "state": "IL",
    "postalCode": "60601",
    "country": "US"
  },
  "type": "distribution",
  "status": "active"
}
```

#### Update Warehouse
```http
PUT /api/v1/warehouses/{id}
Content-Type: application/json

{
  "name": "Updated Warehouse Name"
}
```

#### Delete Warehouse
```http
DELETE /api/v1/warehouses/{id}
```

### Location Endpoints

#### List All Locations
```http
GET /api/v1/locations?warehouseId={warehouseId}&zone={zone}
```

#### Get Location by ID
```http
GET /api/v1/locations/{id}
```

#### Create Location
```http
POST /api/v1/locations
Content-Type: application/json

{
  "warehouseId": "uuid",
  "code": "A-01-02-03",
  "type": "bin",
  "zone": "A",
  "aisle": "01",
  "rack": "02",
  "shelf": "03",
  "status": "active"
}
```

#### Update Location
```http
PUT /api/v1/locations/{id}
Content-Type: application/json

{
  "status": "full"
}
```

## Database

### Schema

The service uses PostgreSQL with the following main tables:
- `warehouses` - Warehouse facility data
- `locations` - Storage locations within warehouses

### Migrations

Database migrations are in `migrations/` directory. Apply them using your migration tool (e.g., Sqitch, Flyway, or custom scripts).

```bash
# Example: Initialize database
psql -U warehouse -d warehouse_db -f migrations/001_init.sql
```

### Sample Migration

```sql
-- migrations/001_init.sql
CREATE TABLE warehouses (
    id UUID PRIMARY KEY,
    code VARCHAR(20) UNIQUE NOT NULL,
    name VARCHAR(200) NOT NULL,
    type VARCHAR(50) NOT NULL,
    status VARCHAR(20) NOT NULL,
    address JSONB NOT NULL,
    metadata JSONB,
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    created_by VARCHAR(100) NOT NULL,
    updated_at TIMESTAMP,
    updated_by VARCHAR(100)
);

CREATE INDEX idx_warehouses_code ON warehouses(code);
CREATE INDEX idx_warehouses_status ON warehouses(status);
```

## Testing

### Unit Tests

```bash
cd build
ctest --output-on-failure

# Run specific test
./bin/warehouse-service-tests "WarehouseTests"

# Run with verbose output
./bin/warehouse-service-tests -s
```

### Integration Tests

```bash
# Start test database
docker-compose -f docker-compose.test.yml up -d

# Run integration tests
cd build
ctest -L integration

# Cleanup
docker-compose -f docker-compose.test.yml down -v
```

### Manual Testing

```bash
# Health check
curl http://localhost:8080/health

# Create warehouse
curl -X POST http://localhost:8080/api/v1/warehouses \
  -H "Content-Type: application/json" \
  -d '{
    "code": "WH-001",
    "name": "Test Warehouse",
    "address": {
      "street": "123 Test St",
      "city": "Chicago",
      "postalCode": "60601",
      "country": "US"
    },
    "type": "distribution",
    "status": "active"
  }'

# Get all warehouses
curl http://localhost:8080/api/v1/warehouses
```

## Development

### Code Style

Follow C++ Core Guidelines and use:
- clang-format for formatting
- clang-tidy for linting
- cppcheck for static analysis

```bash
# Format code
clang-format -i src/**/*.cpp include/**/*.hpp

# Run linter
clang-tidy src/*.cpp -- -I include
```

### Adding New Features

1. Update models in `include/warehouse/models/`
2. Add repository methods in `repositories/`
3. Implement business logic in `services/`
4. Create controller endpoints in `controllers/`
5. Update CMakeLists.txt if adding new files
6. Add tests in `tests/`
7. Update API documentation

### Debugging

```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with GDB
gdb ./bin/warehouse-service

# Run with Valgrind
valgrind --leak-check=full ./bin/warehouse-service
```

## Deployment

### Production Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native"
make -j$(nproc)
strip bin/warehouse-service
```

### Kubernetes

See `k8s/` directory for Kubernetes manifests.

```bash
kubectl apply -f k8s/deployment.yaml
kubectl apply -f k8s/service.yaml
kubectl apply -f k8s/configmap.yaml
```

## Monitoring

### Health Check

```http
GET /health
```

Returns:
```json
{
  "status": "healthy",
  "database": "connected",
  "uptime": 12345
}
```

### Metrics

Metrics are exposed for Prometheus scraping:

```http
GET /metrics
```

### Logging

Logs are written to:
- Console (stdout)
- File: `logs/warehouse-service.log`

Log levels: trace, debug, info, warn, error, critical

## Troubleshooting

### Connection Refused

```bash
# Check if service is running
ps aux | grep warehouse-service

# Check port binding
netstat -tlnp | grep 8080

# Check logs
tail -f logs/warehouse-service.log
```

### Database Connection Issues

```bash
# Test PostgreSQL connection
psql -h localhost -U warehouse -d warehouse_db -c "SELECT 1"

# Check environment variables
env | grep DB_
```

### Build Errors

```bash
# Clean build
rm -rf build && mkdir build && cd build

# Verify dependencies
cmake .. --debug-find

# Check compiler version
g++ --version  # or clang++ --version
```

## Performance

### Benchmarks

Expected performance (on modern hardware):
- **Throughput**: 5,000+ requests/second
- **Latency (p99)**: < 10ms
- **Memory**: ~50-100 MB
- **CPU**: < 5% idle, < 80% under load

### Optimization

- Connection pooling for database
- HTTP keep-alive enabled
- JSON parsing optimized with nlohmann/json
- Async I/O where applicable

## Contributing

See [../../docs/contributing.md](../../docs/contributing.md) for contribution guidelines.

## License

See [LICENSE](../../../LICENSE) in the root directory.

## Support

For issues and questions:
- GitHub Issues: https://github.com/stephenwhippuk/warehouse-management/issues
- Documentation: [../../docs/](../../docs/)

## References

- [Business Entity Contracts](../../../contracts/README.md)
- [Architecture Documentation](../../../docs/architecture.md)
- [C++ Database Migrations](../../../docs/cpp-database-migrations.md)
- [API Documentation](../../../docs/api.md)

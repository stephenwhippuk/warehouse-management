# Product Service

Product master data management service.

## Overview

Manages product/SKU information including name, description, category, and operational status. Products are the master records that inventory levels are tracked against.

## Contracts

- **Fulfils**: Product entity (v1.0)
- **References**: None

## API Documentation

### Endpoints

- `GET /api/v1/products` - List all products (paginated)
- `GET /api/v1/products/active` - List active products (paginated)
- `GET /api/v1/products/{id}` - Get product by ID
- `GET /api/v1/products/sku/{sku}` - Get product by SKU
- `POST /api/v1/products` - Create product
- `PUT /api/v1/products/{id}` - Update product
- `DELETE /api/v1/products/{id}` - Delete product
- `GET /health` - Health check
- `GET /api/swagger.json` - OpenAPI specification
- `GET /api/v1/claims` - Service claims

## Building

```bash
cd product-service
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Running

```bash
# Start with Docker
docker-compose up

# Or run directly
./product-service
```

## Testing

```bash
cd build
ctest -V
```

## Database

Uses PostgreSQL with Sqitch migrations. See [migrations/](migrations/) for schema.

## Configuration

See [config/application.json](config/application.json) for configuration options.

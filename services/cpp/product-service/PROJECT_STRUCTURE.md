# Product Service - Project Structure

## Directory Tree

```
product-service/
├── CMakeLists.txt                   # Main CMake configuration
├── Dockerfile                       # Docker image build
├── docker-compose.yml              # Docker compose with PostgreSQL
├── docker-entrypoint.sh            # Docker entrypoint script
├── README.md                       # Service documentation
├── .gitignore                      # Git ignore rules
│
├── claims.json                     # Service contract claims
│
├── config/                         # Configuration files
│   └── application.json            # Application configuration
│
├── include/product/                # Public headers
│   ├── models/                     # Domain models (internal)
│   │   └── Product.hpp             # Product entity
│   │
│   ├── dtos/                       # Data Transfer Objects (external API)
│   │   ├── ProductItemDto.hpp      # Single product response DTO
│   │   ├── ProductListDto.hpp      # Paginated list response DTO
│   │   └── ErrorDto.hpp            # Standard error response DTO
│   │
│   ├── repositories/               # Data access layer
│   │   └── ProductRepository.hpp   # Product database operations
│   │
│   ├── services/                   # Business logic layer
│   │   └── ProductService.hpp      # Product business logic (returns DTOs)
│   │
│   └── utils/                      # Utility classes
│       └── DtoMapper.hpp           # Model ↔ DTO conversion
│
├── src/                            # Implementation files
│   ├── main.cpp                    # Entry point (stub)
│   │
│   ├── models/
│   │   └── Product.cpp             # Product model implementation
│   │
│   ├── dtos/
│   │   ├── ProductItemDto.cpp      # ProductItemDto implementation
│   │   ├── ProductListDto.cpp      # ProductListDto implementation
│   │   └── ErrorDto.cpp            # ErrorDto implementation
│   │
│   ├── repositories/
│   │   └── ProductRepository.cpp   # Product repository implementation
│   │
│   ├── services/
│   │   └── ProductService.cpp      # Product service implementation
│   │
│   └── utils/
│       └── DtoMapper.cpp           # DtoMapper implementation
│
├── tests/                          # Test files
│   ├── test_main.cpp              # Catch2 main entry point
│   ├── ProductTests.cpp           # Product model tests
│   └── DtoMapperTests.cpp         # DTO validation and mapper tests
│
├── migrations/                     # Database migrations
│   ├── deploy/
│   │   └── 001_init_schema.sql    # Create products table
│   ├── revert/
│   │   └── 001_init_schema.sql    # Drop products table
│   └── verify/
│       └── 001_init_schema.sql    # Verify schema
│
├── sqitch.conf                     # Sqitch configuration
├── sqitch.plan                     # Migration plan
│
└── contracts/                      # Contract definitions
    ├── dtos/
    │   ├── ProductItemDto.json    # Single product DTO contract
    │   ├── ProductListDto.json    # Paginated list DTO contract
    │   └── ErrorDto.json          # Error DTO contract
    │
    ├── requests/
    │   ├── CreateProductRequest.json  # Create product input
    │   └── UpdateProductRequest.json  # Update product input
    │
    └── endpoints/
        ├── GetProducts.json        # GET /api/v1/products
        ├── GetProductById.json     # GET /api/v1/products/{id}
        ├── CreateProduct.json      # POST /api/v1/products
        ├── UpdateProduct.json      # PUT /api/v1/products/{id}
        └── DeleteProduct.json      # DELETE /api/v1/products/{id}
```

## Architecture

### Layer Structure

```
┌─────────────────────────────────────┐
│      HTTP Client Requests           │
└──────────────────┬──────────────────┘
                   │
        ┌──────────▼──────────┐
        │  Controller Layer   │
        │  (Not yet impl'd)   │  - Parse requests
        │  - Work with DTOs   │  - Call services
        └──────────┬──────────┘  - Return JSON
                   │
        ┌──────────▼──────────────┐
        │  Service Layer         │
        │ ProductService         │  - Business logic
        │  - Always returns DTOs  │  - Validation
        │  - Never models         │  - Orchestration
        └──────────┬──────────────┘
                   │
        ┌──────────▼──────────┐     ┌──────────────┐
        │  DtoMapper Utility  │────→│ Model Layer  │
        │                     │     │  Product     │
        │  - Model → DTO      │     │ (internal)   │
        │  - Validation       │     └──────────────┘
        └──────────┬──────────┘
                   │
        ┌──────────▼──────────────┐
        │  Repository Layer      │
        │ ProductRepository      │  - CRUD ops
        │  - Returns models!     │  - DB queries
        │  - NOT DTOs            │  - Mapping
        └──────────┬──────────────┘
                   │
        ┌──────────▼──────────┐
        │   Database Layer    │
        │   PostgreSQL        │
        │   products table    │
        └─────────────────────┘
```

### Data Boundaries

```
External API         Internal
────────────────────────────────
    DTOs      ←→  DtoMapper  ←→  Models
  (JSON)            (utils)       (C++)
             ↓
         Services (Return DTOs)
             ↓
         Repository (Returns Models)
             ↓
         Database
```

## Implemented Components

### ✅ Complete
- **Product Model**: Entity with all fields from contract
- **DTOs**: ProductItemDto, ProductListDto, ErrorDto (fully validated)
- **Validation**: UUID, SKU format, enum values, string lengths
- **DtoMapper**: Model ↔ DTO conversion utility
- **Repository**: Full CRUD operations (database integration stub)
- **Service**: All business operations, returns DTOs
- **Database Schema**: products table with constraints and triggers
- **Tests**: ProductTests.cpp and DtoMapperTests.cpp with 40+ test cases
- **Configuration**: application.json with service config
- **Contracts**: DTOs, Requests, and Endpoint definitions

### 🚧 Partial Implementation
- **main.cpp**: Stub (needs HTTP server setup)
- **Controllers**: Not yet implemented (needs routing)
- **Docker**: Dockerfile created, needs testing
- **HTTP Integration**: Not yet implemented

### 📝 TODO for Full Implementation
1. **HTTP Server Setup**: Add Poco HTTP server and routing
2. **Controllers**: Implement ProductController with all endpoints
3. **Contracts Plugin**: Use contract-plugin for /api/swagger.json and /api/v1/claims
4. **Health Endpoint**: Implement /health for liveness checks
5. **Authentication**: Add service-to-service API key auth
6. **Integration Tests**: HTTP integration tests with real service
7. **Docker Testing**: Test builds and deployments
8. **Configuration Loading**: Load values from environment/config file

## Service Contract Compliance

### Product Entity (Fulfilled)
- ✅ id (UUID) - provided
- ✅ sku (string) - provided  
- ✅ name (string) - provided
- ✅ description (string) - provided
- ✅ category (string) - provided
- ✅ status (enum) - provided

### API Endpoints (Contracted)
- `GET /api/v1/products` - List all products (paginated)
- `GET /api/v1/products/{id}` - Get product by ID
- `POST /api/v1/products` - Create product
- `PUT /api/v1/products/{id}` - Update product
- `DELETE /api/v1/products/{id}` - Delete product

## Building and Testing

### Build
```bash
cd product-service
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Test
```bash
cd build
ctest -V
./product-service-tests "[dto]"  # Run only DTO tests
./product-service-tests "[product]"  # Run product model tests
```

### Run
```bash
# With Docker
docker-compose up

# Direct execution
./build/product-service
```

## Database

Uses PostgreSQL with Sqitch migrations for version control.

### Schema
- `products` table with fields matching Product entity
- Unique constraint on SKU
- Status check constraint (active, inactive, discontinued)
- Automatic updated_at timestamp via trigger

### Migrations
- Deploy: `migrations/deploy/001_init_schema.sql`
- Revert: `migrations/revert/001_init_schema.sql`
- Verify: `migrations/verify/001_init_schema.sql`

## Key Design Decisions

### 1. DTOs Always Returned from Services
**Why**: External API contracts must be immutable and validated. Models can safely be modified internally.

```cpp
// ✅ Correct: Service returns DTO
std::optional<dtos::ProductItemDto> getById(const std::string& id);

// ❌ Wrong: Would expose internal model
std::optional<models::Product> getById(const std::string& id);
```

### 2. DTO Validation at Construction
**Why**: Fail fast - catch invalid data at API boundary before touching database.

```cpp
// Constructor validates UUID, SKU format, enum values, string lengths
ProductItemDto(const std::string& id, const std::string& sku, ...);
```

### 3. Immutable DTOs
**Why**: Prevent accidental modifications that would violate contract invariants.

```cpp
// ✅ Only const getters, no setters
std::string getSku() const { return sku_; }

// ❌ No setters - once created, immutable
// void setSku(const std::string& sku) { sku_ = sku; }
```

### 4. Collection Returns by Const Reference
**Why**: Avoid expensive vector copies on every access.

```cpp
// ✅ Correct: Const reference (zero-cost)
const std::vector<ProductItemDto>& getItems() const { return items_; }

// ❌ Wrong: Value return (expensive copy)
// std::vector<ProductItemDto> getItems() const { return items_; }
```

### 5. Models with toJson/fromJson
**Why**: Central place for model serialization logic, follows contract patterns.

### 6. DtoMapper Utility
**Why**: Single responsibility - only responsible for model → DTO conversion with proper validation.

## Testing Strategy

### Unit Tests (ProductTests.cpp)
- Model construction and validation
- Enum serialization/deserialization
- JSON serialization/deserialization

### DTO Tests (DtoMapperTests.cpp)
- UUID validation (valid/invalid formats)
- SKU format validation
- Enum value conversion
- String length constraints
- Collection pagination
- Mapper conversions (all fields, optional fields)

### Integration Tests (TODO)
- Full HTTP API testing
- Database CRUD operations
- Request/response contracts
- Error handling

## Configuration

See `config/application.json` for:
- Server host/port
- Database connection string and pooling
- Logging levels
- Service metadata
- Authentication keys

## References

- Entity Contract: `/contracts/entities/v1/Product.json`
- DTO Architecture: `/docs/dto-architecture-pattern.md`
- C++ Guidelines: `/docs/cpp-database-migrations.md`

# Contract-Based Swagger Generation

## Overview

The Inventory Service now generates its OpenAPI/Swagger specification automatically from contract definitions instead of being manually coded. This ensures the API documentation stays in sync with the contract system.

## Implementation

### New Components

1. **ContractReader** (`utils/ContractReader.hpp/cpp`)
   - Reads contract JSON files from the `contracts/` directory
   - Parses DTOs, Requests, and Endpoints
   - Converts contract types to JSON Schema types
   - Generates OpenAPI schemas from contract definitions

2. **Updated SwaggerGenerator** (`utils/SwaggerGenerator.hpp/cpp`)
   - New method: `generateSpecFromContracts()` - generates complete OpenAPI spec from contracts
   - Original methods preserved for fallback compatibility
   - Automatic type mapping (UUID → uuid format, PositiveInteger → integer with minimum, etc.)
   - Security scheme generation for API key authentication

3. **Updated SwaggerController** (`controllers/SwaggerController.hpp/cpp`)
   - Uses contract-based generation as primary method
   - Falls back to manual generation if contracts are not found
   - Automatic path detection for contracts directory (supports Docker and local builds)

### Contract Processing

The system processes contracts in the following order:

1. **Load Contracts**
   - DTOs from `contracts/dtos/*.json`
   - Requests from `contracts/requests/*.json`
   - Endpoints from `contracts/endpoints/*.json`

2. **Generate Schemas**
   - Each DTO becomes a component schema
   - Each Request becomes a component schema
   - Entity-prefixed fields are preserved (ProductId, WarehouseCode, etc.)

3. **Generate Paths**
   - Each Endpoint defines an OpenAPI path operation
   - Parameters are mapped by location (Route, Query, Body, Header)
   - Responses include proper status codes and schema references
   - Authentication requirements are added based on endpoint configuration

4. **Type Conversion**
   - Contract types (UUID, DateTime, PositiveInteger, etc.) are converted to JSON Schema types
   - Constraints are preserved (minimum values, formats, enums)
   - References to other DTOs/Requests create `$ref` links

## Test Results

All contract-based generation tests pass successfully:

```
- ContractReader loads DTOs ✓
- ContractReader loads Requests ✓
- ContractReader loads Endpoints ✓
- ContractReader converts types to JSON Schema ✓
- SwaggerGenerator creates spec from contracts ✓
- SwaggerGenerator handles DTO with entity-prefixed fields ✓
```

Statistics:
- 4 DTOs loaded (InventoryItemDto, InventoryListDto, InventoryOperationResultDto, ErrorDto)
- 5 Requests loaded (ReserveInventoryRequest, ReleaseReservationRequest, AllocateInventoryRequest, DeallocateInventoryRequest, AdjustQuantityRequest)
- 7 Endpoints loaded (GetInventoryById, ListInventory, ReserveInventory, ReleaseReservation, AllocateInventory, DeallocateInventory, AdjustInventoryQuantity)
- Generated OpenAPI specification with 7 paths
- 43 assertions passed

## Generated OpenAPI Features

The generated specification includes:

### Security Schemes
```json
{
  "ApiKeyHeader": {
    "type": "apiKey",
    "in": "header",
    "name": "X-Service-Api-Key"
  },
  "ApiKeyAuth": {
    "type": "apiKey",
    "in": "header",
    "name": "Authorization"
  }
}
```

### Schemas
- All DTOs (with entity-prefixed fields for references)
- All Request definitions
- Proper required field declarations
- Type constraints (minimum values, formats, enums)

### Paths
- Proper HTTP methods (GET, POST)
- Path parameters with UUID validation
- Query parameters for filtering
- Request bodies with schema references
- Response definitions with multiple status codes (200, 400, 401, 404, 409, 500)
- Security requirements on endpoints that need authentication

### Tags
- Automatic tag extraction from URIs
- Descriptive tag descriptions

## Endpoint Examples

### GET /api/v1/inventory/{id}
```json
{
  "get": {
    "operationId": "GetInventoryById",
    "summary": "GetInventoryById",
    "description": "Retrieve a single inventory record by ID",
    "tags": ["Inventory"],
    "parameters": [
      {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {"type": "string", "format": "uuid"}
      }
    ],
    "responses": {
      "200": {
        "description": "Inventory found and returned",
        "content": {
          "application/json": {
            "schema": {"$ref": "#/components/schemas/InventoryItemDto"}
          }
        }
      },
      "404": {"$ref": "#/components/schemas/ErrorDto"}
    },
    "security": [
      {"ApiKeyHeader": []},
      {"ApiKeyAuth": []}
    ]
  }
}
```

### POST /api/v1/inventory/{id}/reserve
```json
{
  "post": {
    "operationId": "ReserveInventory",
    "summary": "ReserveInventory",
    "description": "Reserve inventory for an order",
    "tags": ["Inventory"],
    "parameters": [
      {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {"type": "string", "format": "uuid"}
      }
    ],
    "requestBody": {
      "required": true,
      "content": {
        "application/json": {
          "schema": {"$ref": "#/components/schemas/ReserveInventoryRequest"}
        }
      }
    },
    "responses": {
      "200": {"$ref": "#/components/schemas/InventoryOperationResultDto"},
      "400": {"$ref": "#/components/schemas/ErrorDto"},
      "404": {"$ref": "#/components/schemas/ErrorDto"},
       "409": {"$ref": "#/components/schemas/ErrorDto"}
    },
    "security": [
      {"ApiKeyHeader": []},
      {"ApiKeyAuth": []}
    ]
  }
}
```

## Schema Examples

### InventoryItemDto
Includes entity-prefixed fields for referenced entities:
- `ProductId`, `ProductSku`, `ProductName`, `ProductCategory` (from Product entity)
- `WarehouseId`, `WarehouseCode`, `WarehouseName` (from Warehouse entity)
- `LocationId`, `LocationCode`, `LocationAisle`, `LocationBay`, `LocationLevel` (from Location entity)
- Direct Inventory fields without prefix: `id`, `quantity`, `availableQuantity`, etc.

### ReserveInventoryRequest
```json
{
  "type": "object",
  "properties": {
    "quantity": {
      "type": "integer",
      "minimum": 1,
      "description": "Quantity to reserve"
    },
    "orderId": {
      "type": "string",
      "format": "uuid",
      "description": "Order ID for reservation tracking"
    }
  },
  "required": ["quantity", "orderId"]
}
```

## Benefits

1. **Single Source of Truth**: API documentation is generated from the same contracts that define the system
2. **Automatic Consistency**: Changes to contracts automatically update the Swagger spec
3. **Type Safety**: Contract type definitions ensure proper JSON Schema types
4. **Naming Convention Enforcement**: Entity-prefixed fields are preserved in the output
5. **Comprehensive Coverage**: All endpoints, parameters, and responses are documented
6. **Standards Compliance**: Generates valid OpenAPI 3.0 specification
7. **Fallback Support**: Manual generation still available if contracts are unavailable

## Accessing the Swagger Specification

The OpenAPI specification is available at:
- **URL**: `http://localhost:8080/api/swagger.json` (or appropriate host/port)
- **Format**: JSON (OpenAPI 3.0)
- **Usage**: Can be imported into Swagger UI, Postman, or other API tools

## Deployment

### Docker
The contracts directory is copied to `/app/contracts` in the container:
```dockerfile
COPY --from=builder /app/contracts /app/contracts
```

### Local Development
The service searches for contracts in:
1. `contracts/` (current directory)
2. `../../contracts` (from build/bin)
3. `/app/contracts` (Docker)

## Future Enhancements

1. **Contract Validation**: Add build-time validation that all endpoints match claims
2. **Example Generation**: Auto-generate request/response examples from contracts
3. **Health Endpoint**: Add health endpoint contract definition
4. **Multiple Versions**: Support serving multiple API versions simultaneously
5. **Custom Tags**: Allow custom tag definitions in contracts
6. **Extended Types**: Add more contract type mappings as new types are defined
7. **Performance**: Cache parsed contracts to avoid re-reading files on every request

## Maintenance

When adding new contracts:
1. Add DTO/Request/Endpoint JSON files to appropriate directories
2. Swagger specification is automatically regenerated
3. No code changes required in SwaggerController
4. Run tests to verify: `./build/bin/inventory-service-tests "[contracts]"`

# Swagger/OpenAPI Usage Guide for C++ Services

## Overview

All C++ services in this project expose OpenAPI 3.0 specifications for API documentation and client generation. This enables contract-first development and seamless integration between services and frontends.

## Architecture

```
C++ Service (Poco HTTP Server)
  ├── SwaggerGenerator utility → Generates OpenAPI JSON
  ├── SwaggerController → Serves at /api/swagger.json
  └── Models with toJson() → Automatic schema generation
```

## Implementation Pattern

### 1. Create SwaggerGenerator Utility

Location: `include/{service}/utils/SwaggerGenerator.hpp`

```cpp
class SwaggerGenerator {
public:
    static json generateSpec(const std::string& title, const std::string& version);
    static void addEndpoint(json& spec, ...);
    static void addSchema(json& spec, const std::string& name, const json& schema);
    static json createPathParameter(...);
    static json createQueryParameter(...);
    static json createRequestBody(...);
    static json createResponse(...);
};
```

### 2. Create SwaggerController

Location: `include/{service}/controllers/SwaggerController.hpp`

```cpp
class SwaggerController : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest& req, HTTPServerResponse& res) override;
    
private:
    json generateSpecification();
    void addSchemas(json& spec);
    void addEndpoints(json& spec);
};
```

### 3. Register SwaggerController

In your main application or router:

```cpp
// In RequestHandlerFactory
if (uri == "/api/swagger.json") {
    return new SwaggerController();
}
```

### 4. Document All Endpoints

In `SwaggerController::addEndpoints()`:

```cpp
void SwaggerController::addEndpoints(json& spec) {
    // GET endpoint
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/resource/{id}",
        "get",
        "Get resource by ID",
        "Detailed description here",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Resource ID")
        }),
        json(nullptr), // No request body
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Resource")},
            {"404", {{"$ref", "#/components/responses/NotFound"}}}
        },
        {"ResourceTag"}
    );
    
    // POST endpoint
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/resource",
        "post",
        "Create resource",
        "Create a new resource",
        json(nullptr), // No path parameters
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/Resource",
            "Resource data"
        ),
        {
            {"201", utils::SwaggerGenerator::createResponse("Created", "#/components/schemas/Resource")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}}
        },
        {"ResourceTag"}
    );
}
```

### 5. Define Schemas

Map your C++ models to OpenAPI schemas:

```cpp
void SwaggerController::addSchemas(json& spec) {
    utils::SwaggerGenerator::addSchema(spec, "Resource", {
        {"type", "object"},
        {"required", json::array({"id", "name"})},
        {"properties", {
            {"id", {
                {"type", "string"},
                {"format", "uuid"},
                {"description", "Unique identifier"}
            }},
            {"name", {
                {"type", "string"},
                {"minLength", 1},
                {"maxLength", 100}
            }},
            {"createdAt", {
                {"type", "string"},
                {"format", "date-time"}
            }}
        }},
        {"example", {
            {"id", "123e4567-e89b-12d3-a456-426614174000"},
            {"name", "Example Resource"},
            {"createdAt", "2026-02-07T10:00:00Z"}
        }}
    });
}
```

## Best Practices

### 1. Keep OpenAPI in Sync with Implementation

- Update Swagger when adding/modifying endpoints
- Use consistent naming between routes and OpenAPI paths
- Match model properties to JSON Schema contracts

### 2. Use Schema References

```cpp
// Good: Reference shared schemas
{"$ref", "#/components/schemas/Inventory"}

// Avoid: Inline schemas everywhere
{"type", "object", "properties", {...}}
```

### 3. Document All Response Codes

```cpp
{
    {"200", createResponse("Success", "#/components/schemas/Result")},
    {"400", {{"$ref", "#/components/responses/BadRequest"}}},
    {"404", {{"$ref", "#/components/responses/NotFound"}}},
    {"500", {{"$ref", "#/components/responses/InternalError"}}}
}
```

### 4. Use Tags for Organization

```cpp
// Add tags to spec
spec["tags"] = json::array({
    {{"name", "Inventory"}, {"description", "Inventory operations"}},
    {{"name", "Orders"}, {"description", "Order management"}}
});

// Reference in endpoints
utils::SwaggerGenerator::addEndpoint(
    spec, path, method, summary, description,
    parameters, requestBody, responses,
    {"Inventory"} // Tags
);
```

### 5. Provide Examples

```cpp
{"example", {
    {"id", "123e4567-e89b-12d3-a456-426614174000"},
    {"quantity", 100},
    {"status", "available"}
}}
```

## Client Generation

### TypeScript/Vue.js Frontend

```bash
# Generate TypeScript client
openapi-generator-cli generate \
  -i http://localhost:8081/api/swagger.json \
  -g typescript-axios \
  -o src/api/inventory

# Use in Vue
import { InventoryApi } from '@/api/inventory';
const api = new InventoryApi();
const result = await api.getInventoryById(id);
```

### C# Services

```bash
# Using NSwag
nswag openapi2csclient \
  /input:http://inventory-service:8081/api/swagger.json \
  /output:Clients/InventoryClient.cs \
  /namespace:MyApp.Clients

# Use in C#
var client = new InventoryClient(httpClient);
var inventory = await client.GetInventoryByIdAsync(id);
```

### Python Services

```bash
# Using OpenAPI Generator
openapi-generator-cli generate \
  -i http://localhost:8081/api/swagger.json \
  -g python \
  -o clients/inventory

# Use in Python
from inventory import InventoryApi
api = InventoryApi()
inventory = api.get_inventory_by_id(id)
```

## Testing with Tools

### Swagger UI

```bash
# Add to docker-compose.yml
swagger-ui:
  image: swaggerapi/swagger-ui
  ports:
    - "8080:8080"
  environment:
    - SWAGGER_JSON_URL=http://inventory-service:8081/api/swagger.json
```

### Postman

1. Import → Link → `http://localhost:8081/api/swagger.json`
2. Postman creates collection automatically
3. Test endpoints directly

### Bruno/Insomnia

Import OpenAPI URL directly in the tool.

## Validation

### Validate OpenAPI Spec

```bash
# Using swagger-cli
npm install -g @apidevtools/swagger-cli
swagger-cli validate http://localhost:8081/api/swagger.json

# Using openapi-generator
openapi-generator-cli validate -i http://localhost:8081/api/swagger.json
```

## Service Integration

### Example: Order Service Calling Inventory Service

```cpp
// In Order Service

// 1. Generate client from Swagger
// openapi-generator-cli generate -i http://inventory-service:8081/api/swagger.json -g cpp-restsdk

// 2. Use generated client
#include "inventory_api/InventoryApi.h"

void OrderService::createOrder(const Order& order) {
    InventoryApi inventoryApi("http://inventory-service:8081");
    
    // Reserve inventory
    auto result = inventoryApi.reserveInventory(
        inventoryId,
        ReserveRequest(order.quantity)
    );
    
    if (result.availableQuantity < order.quantity) {
        throw std::runtime_error("Insufficient inventory");
    }
}
```

## Contract-First Workflow

1. **Define JSON Schemas** → `/contracts/schemas/v1/resource.schema.json`
2. **Create Models** → C++ classes with `toJson()`/`fromJson()`
3. **Generate OpenAPI** → SwaggerController maps models to schemas
4. **Generate Clients** → Frontend/services consume OpenAPI
5. **Implement Endpoints** → Controllers use models and services

## Troubleshooting

### Swagger JSON Not Loading

```bash
# Check service is running
curl http://localhost:8081/health

# Check Swagger endpoint
curl http://localhost:8081/api/swagger.json

# Check for JSON errors
curl http://localhost:8081/api/swagger.json | jq .
```

### Schema Validation Errors

- Ensure all `$ref` references exist in `components.schemas`
- Check for typos in schema names
- Validate required properties match model implementation

### Client Generation Fails

- Validate OpenAPI spec first
- Check generator version compatibility
- Use OpenAPI 3.0 format (not 2.0/Swagger)

## Migration from Existing Services

If you have an existing C++ service without Swagger:

1. Add SwaggerGenerator utility (copy from inventory-service)
2. Create SwaggerController
3. Update CMakeLists.txt to include new files
4. Add route for `/api/swagger.json`
5. Document existing endpoints in `addEndpoints()`
6. Add schemas for existing models in `addSchemas()`
7. Test with Swagger UI

## References

- OpenAPI 3.0 Specification: https://spec.openapis.org/oas/v3.0.0
- Swagger UI: https://swagger.io/tools/swagger-ui/
- OpenAPI Generator: https://openapi-generator.tech/
- Example Implementation: `services/cpp/inventory-service/src/controllers/SwaggerController.cpp`

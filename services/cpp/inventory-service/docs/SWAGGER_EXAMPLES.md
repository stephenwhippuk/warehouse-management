# Swagger/OpenAPI Examples

## Accessing API Documentation

The inventory service provides OpenAPI 3.0 specification at:

```
GET /api/swagger.json
```

### Development
```bash
curl http://localhost:8081/api/swagger.json | jq
```

### Docker
```bash
curl http://inventory-service:8081/api/swagger.json | jq
```

## Using with Swagger UI

### Option 1: Online Swagger Editor
1. Go to https://editor.swagger.io/
2. File → Import URL
3. Enter: http://localhost:8081/api/swagger.json

### Option 2: Local Swagger UI (Docker)
```bash
docker run -p 8080:8080 -e SWAGGER_JSON_URL=http://host.docker.internal:8081/api/swagger.json swaggerapi/swagger-ui
```
Then open: http://localhost:8080

### Option 3: Add Swagger UI to docker-compose.yml
```yaml
swagger-ui:
  image: swaggerapi/swagger-ui
  ports:
    - "8080:8080"
  environment:
    - SWAGGER_JSON_URL=http://inventory-service:8081/api/swagger.json
  depends_on:
    - inventory-service
```

## Example OpenAPI Output

```json
{
  "openapi": "3.0.0",
  "info": {
    "title": "Inventory Service API",
    "version": "1.0.0",
    "description": "API for managing warehouse inventory, stock levels, and inventory operations"
  },
  "servers": [
    {
      "url": "http://localhost:8081",
      "description": "Development server"
    },
    {
      "url": "http://inventory-service:8081",
      "description": "Docker container"
    }
  ],
  "paths": {
    "/api/v1/inventory": {
      "get": {
        "summary": "List all inventory records",
        "tags": ["Inventory"],
        "parameters": [
          {
            "name": "warehouseId",
            "in": "query",
            "description": "Filter by warehouse ID",
            "required": false,
            "schema": { "type": "string" }
          }
        ],
        "responses": {
          "200": {
            "description": "Success",
            "content": {
              "application/json": {
                "schema": {
                  "$ref": "#/components/schemas/Inventory"
                }
              }
            }
          }
        }
      }
    }
  },
  "components": {
    "schemas": {
      "Inventory": {
        "type": "object",
        "required": ["id", "productId", "warehouseId", "locationId", "quantity"],
        "properties": {
          "id": { "type": "string", "format": "uuid" },
          "productId": { "type": "string", "format": "uuid" },
          "quantity": { "type": "integer", "minimum": 0 }
        }
      }
    }
  }
}
```

## Consuming API Definitions

### From Other Services (C#/.NET)

```csharp
// Install: Swashbuckle.AspNetCore.Cli
// Generate client:
nswag openapi2csclient /input:http://inventory-service:8081/api/swagger.json /output:InventoryClient.cs

// Use generated client:
var client = new InventoryClient(httpClient);
var inventory = await client.GetInventoryByIdAsync(id);
```

### From Frontend (TypeScript/Vue.js)

```bash
# Install OpenAPI generator
npm install -g @openapitools/openapi-generator-cli

# Generate TypeScript client
openapi-generator-cli generate \
  -i http://localhost:8081/api/swagger.json \
  -g typescript-axios \
  -o src/api/inventory

# Use in Vue component:
import { InventoryApi } from '@/api/inventory';

const api = new InventoryApi();
const inventory = await api.getInventoryById(id);
```

### Postman Collection

1. Open Postman
2. Import → Link
3. Enter: http://localhost:8081/api/swagger.json
4. Postman automatically creates collection with all endpoints

### API Testing Tools

```bash
# Bruno (CLI)
bruno import openapi http://localhost:8081/api/swagger.json

# Insomnia
# File → Import → URL → http://localhost:8081/api/swagger.json
```

## Adding New Endpoints

When adding new endpoints, update [SwaggerController.cpp](src/controllers/SwaggerController.cpp):

```cpp
void SwaggerController::addEndpoints(json& spec) {
    // Add your new endpoint
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/custom",
        "post",
        "Custom operation",
        "Detailed description",
        json::array({
            utils::SwaggerGenerator::createQueryParameter("param", "Description")
        }),
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/CustomSchema",
            "Request body description"
        ),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Result")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}}
        },
        {"Inventory"}
    );
}
```

## Contract-First Development

1. **Define JSON Schemas** in `/contracts/schemas/v1/`
2. **Generate OpenAPI** using SwaggerGenerator
3. **Generate Clients** from OpenAPI spec
4. **Implement Backend** matching the contract

This ensures consistency between services and frontends.

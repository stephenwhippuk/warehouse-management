# Business Entity Contracts

This directory contains the canonical schema definitions for all business entities in the warehouse management system. These contracts serve as the **single source of truth** for entity structures across all services and applications.

## Purpose

- **Consistency**: Ensure all services use the same entity definitions
- **Validation**: Provide static validation schemas for data integrity
- **Documentation**: Self-documenting API contracts
- **Code Generation**: Generate types/classes for different languages
- **Versioning**: Track changes to entity schemas over time

## Schema Format

All contracts are defined using [JSON Schema](https://json-schema.org/) (Draft 2020-12), which:
- Is language-agnostic
- Supports validation in C++, C#, JavaScript/TypeScript, and more
- Has extensive tooling for code generation
- Provides human-readable documentation

## Directory Structure

```
contracts/
├── schemas/
│   ├── v1/                    # Version 1 schemas
│   │   ├── product.schema.json
│   │   ├── inventory.schema.json
│   │   ├── order.schema.json
│   │   ├── warehouse.schema.json
│   │   ├── location.schema.json
│   │   ├── user.schema.json
│   │   ├── shipment.schema.json
│   │   └── common.schema.json  # Shared types
│   └── v2/                    # Future versions
├── examples/                  # Example entity instances
└── README.md                  # This file
```

## Using Contracts

### In C++ Services

Use libraries like [nlohmann/json-schema-validator](https://github.com/pboettch/json-schema-validator):

```cpp
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json;
using nlohmann::json_schema::json_validator;

// Load schema
std::ifstream schema_file("contracts/schemas/v1/product.schema.json");
json schema = json::parse(schema_file);

// Create validator
json_validator validator;
validator.set_root_schema(schema);

// Validate data
json product_data = {
    {"id", "PROD-001"},
    {"sku", "WH-WIDGET-001"},
    {"name", "Premium Widget"}
};

validator.validate(product_data); // Throws if invalid
```

### In C# Services

Use [NJsonSchema](https://github.com/RicoSuter/NJsonSchema):

```csharp
using NJsonSchema;
using Newtonsoft.Json.Linq;

// Load and parse schema
var schema = await JsonSchema.FromFileAsync("contracts/schemas/v1/product.schema.json");

// Validate data
var productData = JObject.Parse(@"{
    ""id"": ""PROD-001"",
    ""sku"": ""WH-WIDGET-001"",
    ""name"": ""Premium Widget""
}");

var errors = schema.Validate(productData);
if (errors.Count > 0)
{
    // Handle validation errors
}

// Or generate C# classes
var generator = new CSharpGenerator(schema);
var code = generator.GenerateFile("Product");
```

### In Vue 3 / TypeScript Applications

Use [ajv](https://ajv.js.org/) for validation and generate TypeScript types:

```typescript
import Ajv from 'ajv';
import productSchema from '@/contracts/schemas/v1/product.schema.json';

const ajv = new Ajv();
const validate = ajv.compile(productSchema);

const product = {
  id: 'PROD-001',
  sku: 'WH-WIDGET-001',
  name: 'Premium Widget'
};

if (!validate(product)) {
  console.error('Validation errors:', validate.errors);
}
```

Generate TypeScript types using [json-schema-to-typescript](https://github.com/bcherny/json-schema-to-typescript):

```bash
npx json-schema-to-typescript contracts/schemas/v1/*.schema.json -o apps/shared/types/
```

## Versioning Strategy

### Version 1 (v1)
The current schema version. All services should use v1 schemas by default.

### Service-Specific Extensions
Individual services may extend these contracts with additional fields, but **must not** modify or remove the core schema fields. This ensures interoperability while allowing service-specific customization.

Example service-specific extension:
```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://warehouse-mgmt.example.com/schemas/inventory-service/product-extended.schema.json",
  "title": "Extended Product (Inventory Service)",
  "allOf": [
    { "$ref": "../../contracts/schemas/v1/product.schema.json" },
    {
      "type": "object",
      "properties": {
        "binLocations": {
          "type": "array",
          "items": { "type": "string" }
        }
      }
    }
  ]
}
```

### Schema Evolution

When changes are needed:
1. Minor changes (adding optional fields) → Update v1 schemas
2. Breaking changes → Create v2 schemas
3. Deprecate old versions gradually
4. Document migration paths

## Validation in CI/CD

Add schema validation to your CI/CD pipeline:

```bash
# Validate all example files against schemas
npm install -g ajv-cli
ajv validate -s contracts/schemas/v1/product.schema.json \
             -d "contracts/examples/product-*.json"
```

## Contributing

When adding or modifying schemas:
1. Update the schema file in the appropriate version directory
2. Add example instances to `examples/`
3. Update this README if adding new entities
4. Run validation tests before committing
5. Update service implementations to match schema changes

## Schema Reference

| Entity | Description | Schema File |
|--------|-------------|-------------|
| Product | Product/SKU master data | [product.schema.json](schemas/v1/product.schema.json) |
| Inventory | Stock levels and locations | [inventory.schema.json](schemas/v1/inventory.schema.json) |
| Order | Customer orders and line items | [order.schema.json](schemas/v1/order.schema.json) |
| Warehouse | Warehouse facilities | [warehouse.schema.json](schemas/v1/warehouse.schema.json) |
| Location | Storage locations within warehouses | [location.schema.json](schemas/v1/location.schema.json) |
| User | System users and permissions | [user.schema.json](schemas/v1/user.schema.json) |
| Shipment | Outbound shipments and deliveries | [shipment.schema.json](schemas/v1/shipment.schema.json) |
| Common | Shared types and definitions | [common.schema.json](schemas/v1/common.schema.json) |

## Additional Resources

- [JSON Schema Documentation](https://json-schema.org/learn/)
- [Understanding JSON Schema](https://json-schema.org/understanding-json-schema/)
- [JSON Schema Validator](https://www.jsonschemavalidator.net/)

# DTO Architecture Pattern - Implementation Guide

## Overview

This document describes the Data Transfer Object (DTO) architecture pattern implemented in the warehouse management system. This pattern ensures that **domain models remain internal** to the service and data layers, while **DTOs are exposed** to controllers and external consumers.

## Architecture Principles

### 1. **Separation of Concerns**

```
Controllers (HTTP) → DTOs → Services → Models → Repositories → Database
                      ↑
                  Public API
```

- **Models (Domain Entities)**: Internal representation, business logic, database mapping
- **DTOs**: External representation, validation, serialization, contract compliance
- **Services**: Convert models to DTOs before returning to controllers
- **Controllers**: Work exclusively with DTOs, never touch models directly

### 2. **Contract Compliance**

All DTOs must:
- Match contractual definitions in `/contracts/dtos/`
- Include all required identity fields for referenced entities
- Validate fields using contractual type restrictions
- Use entity-prefixed naming for referenced entity fields (e.g., `ProductId`, `WarehouseName`)
 - Treat contracts as input/output boundaries: internal DB/model field names may differ as long as DtoMapper/request mapping is correct (note derived/aliased fields in claims when practical)

### 3. **Immutability & Validation**

DTOs are:
- **Immutable**: All fields passed in constructor, no setters
- **Validated**: Constructor validates all fields against contract types
- **Serializable**: Provide `toJson()` method for API responses

## Implementation Pattern

### Step 1: Create DTO Directory Structure

```bash
include/{service}/dtos/          # DTO header files
src/dtos/                        # DTO implementation files
```

### Step 2: Define DTO Classes

Each DTO should follow this pattern:

#### Header File (`include/{service}/dtos/{DtoName}.hpp`)

```cpp
#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace {service} {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief {Description}
 * 
 * Conforms to {DtoName} contract v{version}
 */
class {DtoName} {
public:
    /**
     * @brief Construct DTO with all required fields
     * @param {field1} {description} ({type constraint})
     * @param {field2} {description} ({type constraint})
     * ...
     */
    {DtoName}(const std::string& requiredField1,
              int requiredField2,
              const std::optional<std::string>& optionalField = std::nullopt);

    // Getters (immutable - all const, no setters)
    std::string getField1() const { return field1_; }
    int getField2() const { return field2_; }
    std::optional<std::string> getOptionalField() const { return optionalField_; }

    // Serialization
    json toJson() const;

private:
    // Fields
    std::string field1_;
    int field2_;
    std::optional<std::string> optionalField_;

    // Validation (called from constructor)
    void validateUuid(const std::string& uuid, const std::string& fieldName) const;
    void validateNonNegativeInteger(int value, const std::string& fieldName) const;
    void validateDateTime(const std::string& dateTime, const std::string& fieldName) const;
};

} // namespace dtos
} // namespace {service}
```

#### Implementation File (`src/dtos/{DtoName}.cpp`)

```cpp
#include "{service}/dtos/{DtoName}.hpp"
#include <stdexcept>
#include <regex>

namespace {service} {
namespace dtos {

{DtoName}::{DtoName}(
    const std::string& field1,
    int field2,
    const std::optional<std::string>& optionalField)
    : field1_(field1)
    , field2_(field2)
    , optionalField_(optionalField) {
    
    // Validate all required fields
    validateUuid(field1_, "field1");
    validateNonNegativeInteger(field2_, "field2");
    
    // Validate optional fields if present
    if (optionalField_) {
        validateDateTime(*optionalField_, "optionalField");
    }
}

void {DtoName}::validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}

void {DtoName}::validateNonNegativeInteger(int value, const std::string& fieldName) const {
    if (value < 0) {
        throw std::invalid_argument(fieldName + " must be non-negative");
    }
}

void {DtoName}::validateDateTime(const std::string& dateTime, const std::string& fieldName) const {
    if (dateTime.empty()) {
        throw std::invalid_argument(fieldName + " cannot be empty");
    }
    // ISO 8601 format validation
    static const std::regex isoRegex(
        R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$)"
    );
    if (!std::regex_match(dateTime, isoRegex)) {
        throw std::invalid_argument(fieldName + " must be in ISO 8601 format");
    }
}

json {DtoName}::toJson() const {
    json j = {
        {"field1", field1_},
        {"field2", field2_}
    };
    
    // Add optional fields if present
    if (optionalField_) {
        j["optionalField"] = *optionalField_;
    }
    
    return j;
}

} // namespace dtos
} // namespace {service}
```

### Step 3: Create DtoMapper Utility

Create a mapper utility to convert models to DTOs:

#### Header (`include/{service}/utils/DtoMapper.hpp`)

```cpp
#pragma once

#include "{service}/models/{Model}.hpp"
#include "{service}/dtos/{Dto}.hpp"

namespace {service} {
namespace utils {

/**
 * @brief Utility class to convert domain models to DTOs
 */
class DtoMapper {
public:
    /**
     * @brief Convert Model to Dto
     * @param model The domain model
     * @param ... Additional fields from referenced entities
     * @return Dto with all required fields
     */
    static dtos::{Dto} toDto(
        const models::{Model}& model,
        const std::string& referenceEntityField,
        ...);
};

} // namespace utils
} // namespace {service}
```

### Step 4: Update Service Layer

Services should return DTOs instead of models:

#### Service Header Updates

```cpp
// Before: Return models
std::optional<models::Entity> getById(const std::string& id);
std::vector<models::Entity> getAll();

// After: Return DTOs
std::optional<dtos::EntityDto> getById(const std::string& id);
std::vector<dtos::EntityDto> getAll();
```

#### Service Implementation Updates

```cpp
#include "{service}/utils/DtoMapper.hpp"

std::optional<dtos::EntityDto> Service::getById(const std::string& id) {
    auto entity = repository_->findById(id);
    if (!entity) {
        return std::nullopt;
    }
    
    // TODO: Fetch identity fields from referenced entities
    // For now, using placeholders
    return utils::DtoMapper::toEntityDto(
        *entity,
        "REF-" + entity->getReferenceId().substr(0, 8) // Placeholder
    );
}

std::vector<dtos::EntityDto> Service::getAll() {
    auto entities = repository_->findAll();
    std::vector<dtos::EntityDto> dtos;
    dtos.reserve(entities.size());
    
    for (const auto& entity : entities) {
        // TODO: Batch fetch reference data for performance
        dtos.push_back(utils::DtoMapper::toEntityDto(
            entity,
            "REF-" + entity->getReferenceId().substr(0, 8)
        ));
    }
    
    return dtos;
}
```

### Step 5: Update Controllers

Controllers work with DTOs - **no changes needed** if already using `.toJson()`:

```cpp
void Controller::handleGetAll(Response& response) {
    auto dtos = service_->getAll();  // Now returns DTOs
    json j = json::array();
    for (const auto& dto : dtos) {
        j.push_back(dto.toJson());   // DTOs have toJson() method
    }
    sendJsonResponse(response, j.dump());
}

void Controller::handleGetById(const std::string& id, Response& response) {
    auto dto = service_->getById(id);  // Now returns optional DTO
    if (!dto) {
        sendErrorResponse(response, "Not found", 404);
        return;
    }
    sendJsonResponse(response, dto->toJson().dump());
}
```

### Step 6: Update CMakeLists.txt

Add DTO source files to build:

```cmake
set(SOURCES
    # ... existing sources ...
    src/dtos/EntityDto.cpp
    src/dtos/EntityListDto.cpp
    src/dtos/EntityOperationResultDto.cpp
    src/dtos/ErrorDto.cpp
    src/utils/DtoMapper.cpp
)
```

## Common DTOs

### 1. ErrorDto

Standard error response for all services:

**Fields:**
- `error` (string, required): Error type
- `message` (string, required): Human-readable message
- `requestId` (UUID, required): Request identifier for tracing
- `timestamp` (DateTime, required): When error occurred
- `path` (string, required): Request URI path
- `details` (array, optional): Detailed error information

**Usage:**
```cpp
return dtos::ErrorDto(
    "ValidationError",
    "Invalid input data",
    requestId,
    ISO8601::now(),
    request.getURI(),
    std::nullopt
);
```

### 2. EntityItemDto

Detailed view of a single entity with referenced data:

**Fields:**
- All fulfilled entity fields
- Referenced entity identity fields (entity-prefixed)
- Optional cached fields from referenced entities
- Computed fields (e.g., `availableQuantity`)

**Usage:**
```cpp
return dtos::InventoryItemDto(
    inventory.getId(),
    inventory.getProductId(),
    "SKU-12345",        // Product.sku (identity field)
    inventory.getWarehouseId(),
    "WH-001",           // Warehouse.code (identity field)
    // ... rest of fields
);
```

### 3. EntityListDto

Paginated list response:

**Fields:**
- `items` (array of EntityItemDto, required)
- `totalCount` (NonNegativeInteger, required)
- `page` (PositiveInteger, required)
- `pageSize` (PositiveInteger, required)
- `totalPages` (PositiveInteger, required)

**Usage:**
```cpp
return dtos::InventoryListDto(
    items,
    totalCount,
    page,
    pageSize,
    (totalCount + pageSize - 1) / pageSize
);
```

### 4. OperationResultDto

Result of a mutating operation:

**Fields:**
- Entity identity fields
- Key state fields affected by operation
- `operation` (string, required): Operation name
- `operationQuantity` (number, required): Quantity affected
- `success` (boolean, required): Whether succeeded
- `message` (string, optional): Optional message

**Usage:**
```cpp
return dtos::InventoryOperationResultDto(
    inventory.getId(),
    inventory.getProductId(),
    inventory.getQuantity(),
    inventory.getReservedQuantity(),
    inventory.getAllocatedQuantity(),
    inventory.getAvailableQuantity(),
    "reserve",
    quantity,
    true,
    std::nullopt
);
```

## Validation Helpers

### UUID Validation

```cpp
void validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}
```

### NonNegativeInteger Validation

```cpp
void validateNonNegativeInteger(int value, const std::string& fieldName) const {
    if (value < 0) {
        throw std::invalid_argument(fieldName + " must be non-negative");
    }
}
```

### PositiveInteger Validation

```cpp
void validatePositiveInteger(int value, const std::string& fieldName) const {
    if (value < 1) {
        throw std::invalid_argument(fieldName + " must be positive (greater than 0)");
    }
}
```

### DateTime Validation (ISO 8601)

```cpp
void validateDateTime(const std::string& dateTime, const std::string& fieldName) const {
    if (dateTime.empty()) {
        throw std::invalid_argument(fieldName + " cannot be empty");
    }
    static const std::regex isoRegex(
        R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$)"
    );
    if (!std::regex_match(dateTime, isoRegex)) {
        throw std::invalid_argument(fieldName + " must be in ISO 8601 format");
    }
}
```

### Enum Validation

```cpp
void validateStatus(const std::string& status) const {
    static const std::vector<std::string> validStatuses = {
        "active", "inactive", "pending", "completed"
    };
    
    if (std::find(validStatuses.begin(), validStatuses.end(), status) == validStatuses.end()) {
        throw std::invalid_argument("Status must be a valid value");
    }
}
```

## Migration Checklist

When migrating an existing service to use DTOs:

- [ ] **1. Create directory structure**
  - [ ] `include/{service}/dtos/`
  - [ ] `src/dtos/`

- [ ] **2. Define DTO classes**
  - [ ] ErrorDto
  - [ ] EntityItemDto (for each fulfilled entity)
  - [ ] EntityListDto
  - [ ] OperationResultDto (if applicable)

- [ ] **3. Implement DTOs**
  - [ ] Constructor with all fields
  - [ ] Validation for each field type
  - [ ] toJson() method
  - [ ] Const getters (no setters)

- [ ] **4. Create DtoMapper utility**
  - [ ] Header file with conversion methods
  - [ ] Implementation with model → DTO mapping

- [ ] **5. Update Service layer**
  - [ ] Change return types to DTOs in header
  - [ ] Add DtoMapper include
  - [ ] Convert models to DTOs before returning
  - [ ] Add TODOs for fetching reference data

- [ ] **6. Update Controllers** (usually minimal)
  - [ ] Verify `.toJson()` calls work with DTOs
  - [ ] Update stock operations if they return DTOs

- [ ] **7. Update CMakeLists.txt**
  - [ ] Add DTO source files
  - [ ] Add DtoMapper source file

- [ ] **8. Update tests**
  - [ ] Service tests expect DTOs
  - [ ] Mock reference data for DTO construction

- [ ] **9. Build and test**
  - [ ] Run `cmake` and `make`
  - [ ] Run unit tests
  - [ ] Run HTTP integration tests

- [ ] **10. Document**
  - [ ] Update README with DTO information
  - [ ] Add API documentation with DTO schemas

## Benefits

1. **Clear separation**: Models stay internal, DTOs are external contracts
2. **Contract compliance**: DTOs match contractual definitions exactly
3. **Type safety**: Validation at construction time
4. **Immutability**: DTOs cannot be modified after creation
5. **Flexibility**: Can transform models to different DTO shapes for different endpoints
6. **Security**: Only expose what's defined in contracts, hide internal fields
7. **Testability**: Easy to construct DTOs for testing

## Best Practices

1. **Never expose models directly**: Controllers should never see models
2. **Validate in constructor**: Throw exceptions for invalid data
3. **Use const everywhere**: Getters are const, DTOs are immutable
4. **Follow naming conventions**: Entity-prefixed fields for referenced entities
5. **Add TODOs for reference data**: Document where to fetch identity fields
6. **Keep DTOs simple**: No business logic, just data and validation
7. **Reuse validation helpers**: Create common validators for types
8. **Document contracts**: Link to contract JSON files in class comments

## Example: Complete Implementation

See `inventory-service` for a complete reference implementation:

- DTOs: `include/inventory/dtos/`, `src/dtos/`
- Mapper: `include/inventory/utils/DtoMapper.hpp`
- Service updates: `include/inventory/services/InventoryService.hpp`
- Build config: `CMakeLists.txt`

## Common Issues & Solutions

### Issue: "undefined reference to DtoMapper"

**Solution**: Add `src/utils/DtoMapper.cpp` to CMakeLists.txt

### Issue: "cannot convert from models::Entity to dtos::EntityDto"

**Solution**: Use DtoMapper to convert: `utils::DtoMapper::toEntityDto(model, ...)`

### Issue: "optional has no value"

**Solution**: Check for value before dereferencing:
```cpp
if (dto) {
    use(*dto);
}
```

### Issue: "validation exception during DTO construction"

**Solution**: Check that all required fields are provided and match contract types

### Issue: "missing identity fields in DTO"

**Solution**: Fetch identity fields from referenced entities or use placeholders with TODO

## Future Enhancements

1. **Reference data fetching**: Implement service-to-service calls to fetch identity fields
2. **Caching**: Cache referenced entity data to reduce API calls
3. **Batch fetching**: Optimize performance by batch-fetching reference data
4. **DTO versioning**: Support multiple DTO versions for API versioning
5. **Auto-generation**: Generate DTOs from contract JSON schemas

---

**Last Updated**: February 2026  
**Status**: Implemented in inventory-service, ready for rollout to other services

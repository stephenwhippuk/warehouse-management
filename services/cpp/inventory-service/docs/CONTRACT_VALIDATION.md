# Contract Validation System

## Overview

A comprehensive static analyzer for validating service contracts against entity contracts and ensuring code implementation matches contract declarations.

## Components

### 1. ContractValidator (`utils/ContractValidator.hpp/cpp`)

Core validation engine that performs the following checks:

#### **Field Exposure Validation**
- Ensures all fulfilled entity fields are either:
  - Exposed in at least one DTO, OR
  - Marked as `private` in claims.json
- Prevents "hidden" public fields that are never exposed via API

#### **Identity Field Validation**
- Verifies all identity fields from referenced entities are included in DTOs
- Identity fields must be present when a DTO references an entity
- Uses entity-prefixed naming convention (e.g., `ProductSku`, `WarehouseCode`)

#### **DTO Basis Validation**
- Checks that DTO basis declarations reference valid fulfilled or referenced entities
- Ensures `type` field is either "fulfilment" or "reference"
- Validates entities exist in service claims

#### **Request Basis Validation**
- Validates Request basis only includes fulfilled or referenced entities
- Ensures command requests have basis declarations
- Prevents Requests from modifying entities not owned by the service

#### **Naming Convention Validation**
- Verifies entity-prefixed fields follow conventions
- Fields from referenced entities must be prefixed with entity name
- Example: `Product.name` → `ProductName` in DTO

#### **Endpoint Validation**
- Checks response types exist (DTOs or Requests)
- Validates request body types are defined
- Ensures parameter types are valid

### 2. Standalone Validator Tool (`validate-contracts`)

Command-line tool for CI/CD integration:

```bash
./validate-contracts [options]

Options:
  --contracts-root <path>        Global contracts directory
  --service-contracts <path>     Service contracts directory (default: contracts)
  --claims <path>                Path to claims.json (default: claims.json)
  --fail-on-warnings            Exit with error if warnings found
  --json                        Output in JSON format
  --verbose                     Enable verbose logging
```

**Exit Codes:**
- `0`: Validation passed
- `1`: Validation errors found
- `2`: Warnings found (with --fail-on-warnings)
- `3`: Exception/runtime error

### 3. Test Suite (`tests/ContractValidatorTests.cpp`)

Comprehensive tests for all validation rules:
- Field exposure validation
- Identity field checking
- DTO basis validation
- Request basis validation
- Naming convention verification
- Endpoint validation
- Comprehensive validation report

## Current Validation Results

Running the validator on the inventory service reveals **6 errors**:

### Field Exposure Issues (3 errors)

```
[ERROR] field_exposure - Field 'productId' from entity 'Inventory' 
        is marked public but not exposed in any DTO
[ERROR] field_exposure - Field 'warehouseId' from entity 'Inventory' 
        is marked public but not exposed in any DTO
[ERROR] field_exposure - Field 'locationId' from entity 'Inventory' 
        is marked public but not exposed in any DTO
```

**Issue**: These fields are declared in the Inventory entity contract and marked public in claims.json, but the validator cannot find them in any DTO.

**Analysis**: The DTO uses unprefixed names for the Inventory entity fields (e.g., just `id` for the inventory record), while using prefixed names for referenced entities. The validator needs to understand the mapping between entity fields and DTO fields through the `source` attribute.

**Resolution Options**:
1. Update validator to check `source` field in DTOs (maps entity.field to DTO field)
2. Use consistent naming (all unprefixed for fulfilled entity, all prefixed for references)
3. Mark fields as private if intentionally not exposing them

### Identity Field Issues (3 errors)

```
[ERROR] identity_fields - Identity field 'sku' from referenced entity 'Product' 
        is missing in DTO 'InventoryItemDto' (expected 'Productsku')
[ERROR] identity_fields - Identity field 'code' from referenced entity 'Warehouse' 
        is missing in DTO 'InventoryItemDto' (expected 'Warehousecode')
[ERROR] identity_fields - Identity field 'code' from referenced entity 'Location' 
        is missing in DTO 'InventoryItemDto' (expected 'Locationcode')
```

**Issue**: The validator is constructing expected field names by concatenating entity + field name, resulting in incorrect casing ("Productsku" instead of "ProductSku").

**Analysis**: The DTOs actually have these fields with proper casing (`ProductSku`, `WarehouseCode`, `LocationCode`), but the validator's concatenation logic doesn't preserve the field's original casing.

**Resolution**: Fix the validator logic to properly construct entity-prefixed names with correct casing (EntityName + FieldName with capital F).

## Validation Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      Contract Validator                         │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐      ┌──────────────┐     ┌──────────────┐
│   Load       │      │   Load       │     │   Load       │
│   Claims     │      │   Entity     │     │   Service    │
│              │      │   Contracts  │     │   Contracts  │
│ claims.json  │      │   /contracts │     │   contracts/ │
└──────────────┘      │   /entities/ │     │   dtos/      │
                      │   v1/        │     │   requests/  │
                      └──────────────┘     │   endpoints/ │
                                           └──────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐      ┌──────────────┐     ┌──────────────┐
│   Field      │      │   Identity   │     │   DTO Basis  │
│   Exposure   │      │   Fields     │     │   Validation │
└──────────────┘      └──────────────┘     └──────────────┘
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐      ┌──────────────┐     ┌──────────────┐
│   Request    │      │   Naming     │     │   Endpoint   │
│   Basis      │      │   Convention │     │   Validation │
└──────────────┘      └──────────────┘     └──────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  ValidationResult│
                    │  - errors        │
                    │  - warnings      │
                    │  - info          │
                    └──────────────────┘
```

## Usage in CI/CD

### Example GitLab CI/CD

```yaml
stages:
  - validate
  - build
  - test

validate-contracts:
  stage: validate
  script:
    - cd services/cpp/inventory-service
    - ./build/bin/validate-contracts --fail-on-warnings --json > validation-report.json
  artifacts:
    reports:
      junit: validation-report.json
    when: always
```

### Example GitHub Actions

```yaml
name: Contract Validation

on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Build Validator
        run: |
          cd services/cpp/inventory-service
          mkdir -p build && cd build
          cmake .. && make validate-contracts
      
      - name: Run Validation
        run: |
          cd services/cpp/inventory-service
          ./build/bin/validate-contracts --verbose
```

## Next Steps: Code Analysis

The current validator checks contract definitions. The next phase will add **code analysis** to validate actual implementation:

### Planned Features

1. **C++ Code Parser**
   - Parse model class implementations
   - Extract `toJson()` method implementations
   - Verify fields match entity contracts

2. **Controller Analysis**
   - Parse controller methods
   - Verify return types match endpoint DTOs
   - Check parameter types match Request definitions

3. **Repository Analysis**
   - Parse SQL queries
   - Verify columns match entity fields
   - Detect missing/extra fields

4. **Service Method Analysis**
   - Check method signatures match Service Contract operations
   - Verify parameter and return types
   - Validate business logic constraints

### Implementation Approach

Use libclang or a C++ parser to:
- Build AST (Abstract Syntax Tree)
- Extract class definitions
- Parse method bodies
- Match against contract definitions

### Example Validation

```cpp
// Contract says: InventoryItemDto has fields:
// - id (UUID)
// - ProductId (UUID)
// - ProductSku (string)
// - quantity (NonNegativeInteger)

// Code validation checks:
json Inventory::toJson() const {
    return {
        {"id", id_},                    // ✓ Matches contract
        {"ProductId", productId_},      // ✓ Matches contract
        {"ProductSku", productSku_},    // ✓ Matches contract
        {"quantity", quantity_},        // ✓ Matches contract
        {"extraField", extraData_}      // ✗ ERROR: Not in contract!
    };
}
```

## Benefits

1. **Early Detection**: Catch contract violations before runtime
2. **CI/CD Integration**: Fail builds on validation errors
3. **Documentation**: Validation report shows all issues
4. **Consistency**: Enforces contract system rules automatically
5. **Refactoring Safety**: Detect breaking changes to contracts
6. **Compliance**: Ensure all services follow same patterns

## Commands

### Run Tests
```bash
cd services/cpp/inventory-service
./build/bin/inventory-service-tests "[validator]"
```

### Run Standalone Validator
```bash
cd services/cpp/inventory-service
./build/bin/validate-contracts --verbose
```

### JSON Output for Automation
```bash
./build/bin/validate-contracts --json > report.json
```

### Fail on Warnings
```bash
./build/bin/validate-contracts --fail-on-warnings
```

## Files

- `include/inventory/utils/ContractValidator.hpp` - Header
- `src/utils/ContractValidator.cpp` - Implementation
- `src/validate_contracts.cpp` - Standalone tool
- `tests/ContractValidatorTests.cpp` - Test suite
- `docs/CONTRACT_VALIDATION.md` - This document

## Future Enhancements

1. **Custom Rules**: Plugin system for project-specific validation rules
2. **Auto-Fix**: Suggest corrections for common violations
3. **Performance**: Cache parsed contracts for faster validation
4. **Web Dashboard**: Visual reporting of validation results
5. **IDE Integration**: Real-time validation in editors
6. **Multi-Language**: Support for C#, TypeScript services

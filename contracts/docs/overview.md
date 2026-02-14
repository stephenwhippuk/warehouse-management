# Overview of the contract system 

The contract system is designed to provide control over and discoverability of data and apis across systems to track and provide consistency across systems

## contracts and claims

- a contract is a global definition that represents an abstract entity to guide and validate the inputs and outputs of systems. There are 3 types of contract, Type, Entity and Service

- A claim is provided by a service as a promise to provide input and output that matches a contract. Claim types are Fulfilment, Reference, Dto, Request, Event and Endpoint
- Contracts define system inputs/outputs only. Internal DB/model naming can differ as long as requests and DTOs map correctly to the contract. When internal names differ, treat those fields as derived/aliased in claims when practical.

contracts and related claims are all versioned, a given claim only refers to specified versions of a contract, BUT may where applicable support multiple versions 

## Contract Versioning

**Breaking by Default**:
- All new contract versions should be considered **breaking by default**
- No automatic compatibility inference between versions
- Any change (adding, removing, or modifying fields/constraints) creates a new version

**Version Support**:
- Services explicitly declare which contract versions they support via their claims
- A Fulfilment, Reference, or other claim specifies the exact version(s) it implements
- Services may support multiple versions simultaneously (e.g., v1.0, v1.1, v2.0)
- It is the **service owner's responsibility** to determine version compatibility

**No Automatic Upgrades**:
- Systems must explicitly add support for new versions
- Version adoption is a deliberate decision, not automatic
- Allows services to carefully evaluate impact before migrating

**Versioning Strategy**:
- Use semantic versioning (major.minor.patch)
- Major version changes indicate significant restructuring
- Minor/patch versions still considered breaking unless explicitly claimed compatible by the service
- Services declare supported versions in their claim definitions

**Example**:
```
Inventory Service Fulfilment Claim:
  - contract: Inventory
  - versions: ["1.0", "1.1", "2.0"]
  - status: fulfilled
``` 

## contract types

### Type
a type defines the format and size and any other details such as encryption etc that define a specific named subtype for a field.
primitive types of string, number, boolean, symbol are provided 
types may be structure types, providing a compound definition using existing types for elements. Also types may define collections, which are list, dictionary or enumeration. 

important: contractual types are concrete and non-generic. These make no sense in contracts and are excluded. If these need to be mapped, then explicit derivations must be provided.

#### Type Extension and Constraints
Types are built upon a base primitive type and extended through constraints:

**Base Field**: Every type specifies a base type (string, number, boolean, symbol)

**Constraints**: Additional validation rules applied to the base type:
- **Format**: Pattern or format specification (e.g., uuid, email, iso8601-datetime, url)
- **Size**: Length or value constraints
  - For strings: minLength, maxLength, exactLength
  - For numbers: minimum, maximum, multipleOf
- **Pattern**: Regular expression for string validation
- **Enum**: Restricted set of allowed values
- **Encryption**: Whether the field must be encrypted at rest/in transit

**Examples**:
- `UUID`: base = string, format = uuid, exactLength = 36
- `Email`: base = string, format = email, maxLength = 255, pattern = email-regex
- `DateTime`: base = string, format = iso8601-datetime
- `PositiveInteger`: base = number, minimum = 1, multipleOf = 1
- `Percentage`: base = number, minimum = 0, maximum = 100
- `Status`: base = string, enum = ["active", "inactive", "pending"]

#### Structured Type Validation Rules
Structured types provide compound definitions composed of multiple fields:

**Nullability**:
- Structured types **cannot have nullable elements** when providing a value
- All elements within a structured type must be present and non-null
- In entity definitions, the structured type field itself may be nullable (if not Identity or Core)
- Example: An `Address` type cannot have a null `street`, but an entity's `shippingAddress` field can be null

**Element Types**:
- Each element must be either a **primitive type** or an **externally defined type**
- No anonymous types are permitted - all types must be explicitly named and defined
- Elements can reference other structured types (nesting is permitted)

**Nesting**:
- Structured types can contain other structured types as elements
- All nested types must be defined separately with their own type definition
- Example: `OrderAddress` (structured type) can contain `GeoLocation` (structured type), but both must be independently defined

**Example**:
```
Type: Address (structured)
  - street: string (required)
  - city: string (required)
  - postalCode: PostalCode (required, references external type)
  - coordinates: GeoLocation (required, references structured type)

Type: GeoLocation (structured)
  - latitude: number (required)
  - longitude: number (required)
```

#### Collection Type Rules
Collections (list, dictionary, enumeration) follow the same principle as structured types:

**Type References**:
- Collections must reference a **single named type** for their elements
- Valid: `List<Product>`, `Dictionary<string, Address>`, `List<UUID>`
- Invalid: `List<Dictionary<string, Product>>` (nested collection requires intermediate type definition)

**Nested Collections**:
- To nest collections, define an explicit intermediate type
- Example: For a list of product dictionaries:
  1. Define `ProductDictionary` as `Dictionary<string, Product>`
  2. Then define `List<ProductDictionary>`

**No Anonymous Types**:
- Collection elements must reference explicitly named types
- Cannot use inline type definitions within collections
- All collection element types must be defined separately

**Example**:
```
Type: ProductMap (dictionary)
  - key: string
  - value: Product

Type: StoreInventory (structured)
  - storeId: UUID (required)
  - products: ProductMap (required, references dictionary type)

Type: InventoryList (list)
  - element: StoreInventory
``` 

### Entity
Entities represent Global concepts such as Product, Supplier etc, that describe entities that will be used in/referenced by multiple systems. 

Entity contracts have a name and an Owner. The Owner is the system which provides the single source of truth for data of this type. 

Entities consist of fields, that are typed using contractual types. Field specifiers may override existing types so long as the contract is more restrictive, for instance it can add but not remove encryption if needed. 
in addition a field is defined as identity, Core or Complete. 

Identity fields, represents information that must be shared between systems when a DTO is passed around, 

A core field represents a required field that must be provided when creating this data, or can be derived by the system, either way its data that is required to exist for all entities of this type 

A complete field is data that may or may not be present for any instance of the entity, but represents part of the complete global specification. That is it may or may not be present but if it is present it must match the specified type definition.

### Service 
A service contract is the formal contract for systems functionality. This is not a formal API document rather its a definition of functionality that must be provided by the system and which can later be tied to claims that specific systems implement. A Service contract is only required for systems that have a 'public' api, however may be specified for sub systems as well.

**Service Contract Components**:
- **Name**: The service identifier (e.g., InventoryManagementService)
- **Owner**: The team or system responsible for the service
- **Operations**: Collection of functional capabilities the service must provide
- **Dependencies**: Other services or entities required for operation

**Operations Definition**:
Each operation specifies:
- **Name**: Operation identifier (e.g., ReserveStock, GetInventory)
- **Type**: Query (read-only) or Command (state-changing)
- **Required**: Whether the operation must be implemented (required/optional)
- **Input Contract**: Expected request parameters
- **Output Contract**: Expected response format
- **Side Effects**: Which entities will be affected

**Partial Fulfilment**:
- Services may implement a subset of operations (partial fulfilment)
- Each operation's implementation status must be declared
- Optional operations may be omitted
- Required operations must be implemented for full compliance

**Example**:
```
Service Contract: InventoryManagementService v1.0
  - owner: Inventory Team
  - operations:
      - GetInventory: type=Query, required=true
      - ListInventory: type=Query, required=true
      - ReserveStock: type=Command, required=true
      - AllocateStock: type=Command, required=true
      - ReleaseReservation: type=Command, required=false
      - BulkUpdate: type=Command, required=false
``` 

## Claim Types

### Fulfilment
a system must provide a fulfilment object for each entity that it owns. A fulfilment object has a contract field that has a target which targets a list of contractual versions of the entity contract it is fulfilling, an overall status that is fulfilled, partial or absent. 
it then specifies a definition for each field in the contract. For each field it should, provide a method that is direct or derived, a status that is provided or absent and security object that has two elements, access (private/public) and encrypt(boolean). 
**Status Levels**:

*Overall Status* (applies to the entire contract):
- **fulfilled**: All fields in the contract are implemented (status = provided)
- **partial**: Some fields are implemented, but not all fields in the contract
- **absent**: The contract is declared but no fields are currently implemented

*Field Status* (applies to individual fields):
- **provided**: The field is available in this service's implementation
- **absent**: The field is not currently implemented by this service

**Field Method** (how the field is sourced):
- **direct**: The field is stored directly in the service's database in a form that matches the contract type
- **derived**: The field value is computed/derived by the service at runtime (through calculation, lookup, aggregation, etc.) and provided in DTOs that match the contract, but is not stored in this exact form

**Security Access Levels**:
- **public**: Field is exposed in one or more DTOs via the service's API
- **private**: Field exists in the service but is NOT exposed in any DTO (internal use only, must be declared in claims manifest)

**When to Use Partial**:
- Service is implementing a contract incrementally
- Some contract fields are not yet relevant for the service's use cases
- Service is migrating to a new contract version and hasn't completed all fields

**Example**:
```
Inventory Service Fulfilment:
  - contract: Product v1.0
  - overallStatus: partial
  - fields:
      - id: status=provided, method=direct
      - name: status=provided, method=direct
      - description: status=provided, method=direct
      - totalStock: status=provided, method=derived (calculated from inventory records)
      - lastRestockDate: status=provided, method=derived (computed from inventory history)
      - specifications: status=absent (not yet implemented)
      - certifications: status=absent (not yet implemented)
```

**Field Exposure Validation**:
For fulfilments, every field with status=provided must either:
1. Appear in at least one service Dto, OR
2. Be marked with security.access=private

This ensures fulfilled contracts are fully accessible (all public fields exposed) while allowing services to keep certain fields internal.

### Reference
If a system doesn't own an entity, but holds reference to it that must be communicated as part of its own input/output then it should provide a reference claim. A reference claim, simply provides the contract field which targets a list of contractual versions that it supports.

**Reference Requirements**:
- Services must declare which entity contracts they reference
- Must specify which versions of the entity contract are supported
- Must include **all Identity fields** from the referenced entity in their DTOs
- May optionally include additional fields from the entity (for denormalization/caching)

**Field Specification**:
- **Identity Fields**: Always required - all identity fields must be included
- **Additional Fields**: Optional - services may include other fields for performance/caching
- **Consistency**: Referenced fields must match the entity contract's type definition

**Referential Integrity**:
- Services do not validate referenced entity existence (that's the owner's responsibility)
- Services may cache referenced entity data for performance
- Services must handle scenarios where referenced entities may not exist

**Example**:
```
Inventory Service Reference Claim:
  - contract: Product v1.0, v1.1
  - requiredFields: [id, sku] (identity fields)
  - optionalFields: [name, category] (cached for display)
  - usage: "Inventory records reference Product for tracking"

Order Service Reference Claim:
  - contract: Product v1.0
  - requiredFields: [id, sku]
  - optionalFields: [name, price] (cached at order time)
  - contract: Warehouse v1.0
  - requiredFields: [id, code]
  - optionalFields: [name, address]
```

### Dto
The outputs of the system are specified as Dto, which represent the results of API calls and the content of any messages. A Dto has a basis field which should reference any fulfilment or references that the data will include. Each field is then defined in terms of contractual types. There is a naming convention here that should be applied. Any field that references a field in a fulfilment or referenced Contract should be \<Entity\>Name e.g. ProductId. if it doesn't reference a contract it should lack such a Prefix. For each referenced Entity ALL identity fields must be included.

**Basis Field**:
- **Mandatory** for DTOs that include entity data
- References the Fulfilment or Reference claims this DTO is based on
- Declares which entity contracts contribute to this DTO's structure
- May reference multiple entities (e.g., Order DTO includes Product and Warehouse references)

**Field Naming Convention**:
- **Entity-prefixed**: Fields from entity contracts use `<Entity><FieldName>` (e.g., `ProductId`, `ProductSku`, `WarehouseName`)
- **Unprefixed**: Computed or DTO-specific fields lack entity prefix (e.g., `totalPrice`, `itemCount`, `status`)
- **Required Identity Fields**: All identity fields from referenced entities must be included

**Additional Fields**:
- DTOs may include fields not in their basis (computed, aggregated, or service-specific)
- These fields do not require entity prefixes
- Must still be defined using contractual types

**Example**:
```
Dto: InventoryItemDto
  - basis: [Inventory (fulfilment), Product (reference), Warehouse (reference), Location (reference)]
  - fields:
      - id: UUID (from Inventory identity)
      - ProductId: UUID (from Product identity - required)
      - ProductSku: string (from Product identity - required)
      - ProductName: string (from Product - optional cached field)
      - WarehouseId: UUID (from Warehouse identity - required)
      - WarehouseCode: string (from Warehouse identity - required)
      - LocationId: UUID (from Location identity - required)
      - quantity: PositiveInteger (from Inventory)
      - reservedQuantity: PositiveInteger (from Inventory)
      - availableQuantity: PositiveInteger (computed, not from basis)
      - lastUpdated: DateTime (from Inventory)
```     

### Request
A request is a command or query that defines the input parameters needed for a specific ApiCall. These parameters are defined in terms of Contract types. A Command has type field that is (Create,Update, Delete, Process), A basis which indicates which entity(s) will be expected to be affected, referenced and a result type that is a Dto. A Query Object is the same but lacks the type field.

**Basis Field** (for Commands):
- The basis field is an **enforceable expectation**, not just documentation
- Lists which entity(ies) the request will modify or affect
- Validation **must enforce** that the request only modifies entities declared in the basis
- Prevents unintended side effects and maintains contract boundaries
- If a request attempts to modify entities not in its basis, validation should fail

**Command Types**:
- **Create**: Creates new instance(s) of the entity
- **Update**: Modifies existing instance(s) of the entity
- **Delete**: Removes instance(s) of the entity
- **Process**: Performs operations that may affect the entity (e.g., reserve, allocate, transfer)

**Example**:
```
Command: ReserveInventory
  - type: Process
  - basis: [Inventory] (only Inventory entities will be modified)
  - parameters:
      - inventoryId: UUID
      - quantity: PositiveInteger
  - resultType: InventoryOperationResult (Dto)

Command: CreateOrder
  - type: Create
  - basis: [Order, Inventory] (creates Order, affects Inventory)
  - parameters:
      - orderData: OrderCreateRequest
  - resultType: OrderDto
```

## Contract Discovery and Registry

**Purpose**:
- Services must be able to discover available contracts and claims
- System needs to validate that service implementations match their claims
- Enable tooling to generate code, documentation, and validation

**Registry Structure**:
- **Centralized Registry**: Central repository of all contracts and claims
- **Location**: `/contracts` directory in repository, versioned alongside code
- **Format**: JSON files organized by type and version

**Contract Publication**:
- Contracts are defined and versioned in the registry
- Changes follow standard code review process
- Breaking changes require major version increment

**Claim Declaration**:
- Each service maintains a `claims.json` manifest in its service directory
- Lists all contracts the service implements (fulfilments, references)
- Declares supported versions for each contract
- Marks any private fields that are not exposed via DTOs
- Updated as part of service development

**Service-Specific Definitions**:
- Each service defines its own DTOs, Requests, Events, and Endpoints in a `contracts/` subdirectory
- Location: `/services/<language>/<service-name>/contracts/`
- Structure mirrors global contracts but contains service-specific implementations
- These definitions reference global entity contracts and types
- Organized by type: `dtos/`, `requests/`, `events/`, `endpoints/`

**Discovery Mechanism**:
- **Build-time**: Services validate against contracts during compilation
- **Runtime**: Services can query registry for contract details (optional)
- **Tooling**: CI/CD validates claims match implementation

**Validation Timing**:
- **Pre-commit**: Local validation of contract syntax
- **CI Build**: Validate service claims against contracts
  - Verify all fulfilled entity fields are exposed in at least one DTO or marked private
  - Verify DTO basis references match declared fulfilments and references
  - Verify Request basis entities are fulfilled or referenced by the service
- **Integration Tests**: Validate actual API responses match DTOs
- **Runtime** (optional): Validate requests/responses in API gateway

**Example Directory Structure**:
```
/contracts/
  /types/
    /v1/
      UUID.json
      Email.json
      DateTime.json
  /entities/
    /v1/
      Product.json
      Inventory.json
      Warehouse.json
  /services/
    /v1/
      InventoryManagementService.json
  /dtos/
    /v1/
      InventoryItemDto.json
      InventoryListDto.json
  /requests/
    /v1/
      ReserveInventoryRequest.json
  /events/
    /v1/
      InventoryReserved.json

/services/cpp/inventory-service/
  claims.json
  /contracts/
    /dtos/
      InventoryItemDto.json
      InventoryListDto.json
      InventoryOperationResultDto.json
    /requests/
      ReserveInventoryRequest.json
      AllocateInventoryRequest.json
    /events/
      InventoryReserved.json
      InventoryAllocated.json
    /endpoints/
      GetInventory.json
      ListInventory.json
      ReserveInventory.json
  /src/
  /include/

/services/cpp/warehouse-service/
  claims.json
  /contracts/
    /dtos/
    /requests/
    /events/
    /endpoints/
  /src/
  /include/
```

**Example claims.json**:
```json
{
  "service": "inventory-service",
  "version": "1.0.0",
  "fulfilments": [
    {
      "contract": "Inventory",
      "versions": ["1.0"],
      "status": "fulfilled"
    }
  ],
  "references": [
    {
      "contract": "Product",
      "versions": ["1.0", "1.1"]
    },
    {
      "contract": "Warehouse",
      "versions": ["1.0"]
    },
    {
      "contract": "Location",
      "versions": ["1.0"]
    }
  ],
  "serviceContracts": [
    {
      "contract": "InventoryManagementService",
      "versions": ["1.0"],
      "status": "fulfilled"
    }
  ]
}
```

## Validation Strategy

**Contract Fulfilment Completeness**:
- **Critical Rule**: For any entity contract a service fulfills, at least one DTO must expose ALL fields from that contract, OR the service must explicitly declare fields as private in its claims manifest
- This ensures fulfilled contracts are fully accessible through the service's API
- Does NOT apply to referenced entities (only fulfilments)
- Validated at build time

**Field Exposure Requirements** (for Fulfilments only):
```
For each fulfilled entity contract:
  For each field in the contract:
    - Field is included in at least ONE DTO, OR
    - Field is marked as "access: private" in the fulfilment claim
    
If neither condition is met → Build validation fails
```

**Example Validation**:
```
Inventory Entity Contract has fields: [id, productId, warehouseId, locationId, quantity, reservedQuantity, allocatedQuantity, internalNotes, createdAt, updatedAt]

Service DTOs expose: [id, productId, warehouseId, locationId, quantity, reservedQuantity, allocatedQuantity, createdAt, updatedAt]

Claims manifest marks as private: [internalNotes]

Validation: ✓ All fields accounted for (9 in DTOs + 1 private = 10 total)
```

**Validation Levels**:
- **Strict**: All fields must match exactly; unknown fields cause validation failure
- **Lenient**: Additional fields allowed (open-world assumption); only validate known fields
- **Warn-only**: Log validation violations but don't fail requests

**Where Validation Occurs**:

*Build-time Validation*:
- Contract definitions validated for correctness (syntax, type references)
- Service claims validated against contracts
- Generated code matches contracts (type safety)

*API Layer Validation*:
- Input requests validated against Request contracts
- Required fields present and correct types
- Constraint validation (min/max, patterns, enums)
- Returns 400 Bad Request with detailed errors

*Service Layer Validation*:
- Business rule validation (beyond contract constraints)
- Cross-entity validation (e.g., foreign keys exist)
- Authorization checks

*Output Validation* (optional):
- Validate responses match Dto contracts before sending
- Useful in development/testing
- May be disabled in production for performance

**Validation Mode by Environment**:
- **Development**: Strict validation on input and output, fail fast
- **Testing**: Strict validation with detailed error reporting
- **Staging**: Lenient validation with warnings logged
- **Production**: Lenient input validation (backward compatibility), no output validation

**Handling Unknown Fields**:
- **Strict mode**: Reject with 400 error listing unknown fields
- **Lenient mode**: Ignore unknown fields, process known fields
- **Forward compatibility**: Clients may send new fields (future versions)

**Validation Error Response**:
```json
{
  "error": "ValidationError",
  "message": "Request validation failed",
  "details": [
    {
      "field": "quantity",
      "constraint": "minimum",
      "expected": 1,
      "actual": -5,
      "message": "quantity must be at least 1"
    },
    {
      "field": "productId",
      "constraint": "format",
      "expected": "uuid",
      "actual": "not-a-uuid",
      "message": "productId must be a valid UUID"
    }
  ],
  "requestId": "550e8400-e29b-41d4-a716-446655440000"
}
```

**Performance Considerations**:
- Cache compiled validation schemas
- Validate only once per request (at API boundary)
- Use native type validation where possible (C++ types)
- Skip validation for trusted internal service-to-service calls (optional)

## Error Handling

**Standard Error Response Contract**:
All services must return errors in a consistent format:

```json
{
  "error": "<ErrorType>",
  "message": "<Human-readable message>",
  "details": [<Optional array of detail objects>],
  "requestId": "<UUID for tracing>",
  "timestamp": "<ISO8601 DateTime>",
  "path": "<Request URI>"
}
```

**Error Types**:
- **ValidationError**: Contract validation failed (400)
- **NotFoundError**: Requested entity not found (404)
- **ConflictError**: Operation conflicts with current state (409)
- **UnauthorizedError**: Authentication failed or missing (401)
- **ForbiddenError**: Authenticated but not authorized (403)
- **BusinessRuleError**: Business logic violation (400 or 409)
- **InternalError**: Unexpected server error (500)
- **ServiceUnavailableError**: Service temporarily down (503)

**Contract Version Mismatch**:
- Use HTTP 406 Not Acceptable for unsupported contract versions
- Include `Accept-Version` header in requests to specify desired version
- Include `Content-Version` header in responses to indicate returned version

**Version Mismatch Response**:
```json
{
  "error": "VersionNotSupported",
  "message": "The requested contract version is not supported",
  "details": [
    {
      "contract": "Inventory",
      "requestedVersion": "2.5",
      "supportedVersions": ["1.0", "1.1", "2.0"]
    }
  ],
  "requestId": "550e8400-e29b-41d4-a716-446655440000"
}
```

**Error Detail Objects**:
For ValidationError, provide structured details:
```json
{
  "field": "<field path using dot notation>",
  "constraint": "<constraint that failed>",
  "expected": "<expected value or format>",
  "actual": "<actual value received>",
  "message": "<specific error message>"
}
```

## Migration and Deprecation

**Contract Lifecycle**:
1. **Draft**: Contract proposed, under review
2. **Active**: Contract published and in use
3. **Deprecated**: Contract marked for eventual removal
4. **Obsolete**: Contract no longer supported

**Deprecation Process**:
1. Announce deprecation with timeline (minimum 6 months notice)
2. Mark contract version as deprecated in registry
3. All endpoints return `Deprecated: true` header with sunset date
4. Monitor usage of deprecated versions
5. Communicate sunset date (at least 3 months before removal)
6. Remove support after grace period

**Deprecation Headers**:
```
Deprecation: true
Sunset: Sat, 31 Dec 2026 23:59:59 GMT
Link: </api/v2/inventory>; rel="alternate"; version="2.0"
```

**Migration Strategy**:
- Services support both old and new versions during transition
- Provide migration guides for consumers
- Automated tooling to help upgrade (if possible)
- Gradual rollout of new versions

**Breaking Change Communication**:
- Document all breaking changes in CHANGELOG
- Notify all known consumers via email/chat
- Provide migration examples and code samples
- Offer support during migration period

**Version Support Guidelines**:
- Support at minimum: current version + 1 previous major version
- Minor versions: support latest minor of each major version
- Security fixes: backport to all supported versions
- Bug fixes: only in current versions

## Complete End-to-End Example

This example shows a complete contract chain for inventory management:

### 1. Type Definitions

```json
// UUID.json (v1.0)
{
  "name": "UUID",
  "version": "1.0",
  "base": "string",
  "constraints": {
    "format": "uuid",
    "pattern": "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$",
    "exactLength": 36
  }
}

// PositiveInteger.json (v1.0)
{
  "name": "PositiveInteger",
  "version": "1.0",
  "base": "number",
  "constraints": {
    "minimum": 1,
    "multipleOf": 1
  }
}
```

### 2. Entity Contract

```json
// Inventory.json (v1.0)
{
  "name": "Inventory",
  "version": "1.0",
  "owner": "inventory-service",
  "fields": [
    {
      "name": "id",
      "type": "UUID",
      "classification": "identity",
      "required": true,
      "description": "Unique identifier for inventory record"
    },
    {
      "name": "productId",
      "type": "UUID",
      "classification": "identity",
      "required": true,
      "description": "Reference to Product entity"
    },
    {
      "name": "warehouseId",
      "type": "UUID",
      "classification": "core",
      "required": true,
      "description": "Which warehouse holds this inventory"
    },
    {
      "name": "locationId",
      "type": "UUID",
      "classification": "core",
      "required": true,
      "description": "Specific location within warehouse"
    },
    {
      "name": "quantity",
      "type": "PositiveInteger",
      "classification": "core",
      "required": true,
      "description": "Total quantity on hand"
    },
    {
      "name": "reservedQuantity",
      "type": "integer",
      "classification": "core",
      "required": true,
      "constraints": {
        "minimum": 0
      },
      "description": "Quantity reserved for orders"
    },
    {
      "name": "allocatedQuantity",
      "type": "integer",
      "classification": "complete",
      "required": false,
      "constraints": {
        "minimum": 0
      },
      "description": "Quantity allocated for shipment"
    },
    {
      "name": "createdAt",
      "type": "DateTime",
      "classification": "complete",
      "required": true
    },
    {
      "name": "updatedAt",
      "type": "DateTime",
      "classification": "complete",
      "required": true
    }
  ]
}
```

### 3. Service Fulfilment Claim

```json
// inventory-service/claims.json
{
  "service": "inventory-service",
  "version": "1.0.0",
  "fulfilments": [
    {
      "contract": "Inventory",
      "versions": ["1.0"],
      "status": "fulfilled",
      "fields": [
        {
          "name": "id",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "productId",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "warehouseId",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "locationId",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "quantity",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "reservedQuantity",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "allocatedQuantity",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "createdAt",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "updatedAt",
          "status": "provided",
          "method": "direct",
          "security": {"access": "public", "encrypt": false}
        },
        {
          "name": "internalNotes",
          "status": "provided",
          "method": "direct",
          "security": {"access": "private", "encrypt": false},
          "description": "Not exposed in any DTO - internal use only"
        }
      ]
    }
  ],
  "references": [
    {
      "contract": "Product",
      "versions": ["1.0"],
      "requiredFields": ["id", "sku"],
      "optionalFields": ["name"]
    },
    {
      "contract": "Warehouse",
      "versions": ["1.0"],
      "requiredFields": ["id", "code"],
      "optionalFields": []
    },
    {
      "contract": "Location",
      "versions": ["1.0"],
      "requiredFields": ["id"],
      "optionalFields": ["aisle", "bay", "level"]
    }
  ]
}
```

### 4. Dto Definition

```json
// InventoryItemDto.json (v1.0)
{
  "name": "InventoryItemDto",
  "version": "1.0",
  "basis": [
    {"entity": "Inventory", "type": "fulfilment"},
    {"entity": "Product", "type": "reference"},
    {"entity": "Warehouse", "type": "reference"},
    {"entity": "Location", "type": "reference"}
  ],
  "fields": [
    {"name": "id", "type": "UUID", "required": true, "source": "Inventory.id"},
    {"name": "ProductId", "type": "UUID", "required": true, "source": "Product.id"},
    {"name": "ProductSku", "type": "string", "required": true, "source": "Product.sku"},
    {"name": "ProductName", "type": "string", "required": false, "source": "Product.name"},
    {"name": "WarehouseId", "type": "UUID", "required": true, "source": "Warehouse.id"},
    {"name": "WarehouseCode", "type": "string", "required": true, "source": "Warehouse.code"},
    {"name": "LocationId", "type": "UUID", "required": true, "source": "Location.id"},
    {"name": "LocationAisle", "type": "string", "required": false, "source": "Location.aisle"},
    {"name": "quantity", "type": "PositiveInteger", "required": true, "source": "Inventory.quantity"},
    {"name": "reservedQuantity", "type": "integer", "required": true, "source": "Inventory.reservedQuantity"},
    {"name": "allocatedQuantity", "type": "integer", "required": true, "source": "Inventory.allocatedQuantity"},
    {"name": "availableQuantity", "type": "integer", "required": true, "source": "computed", "description": "quantity - reservedQuantity - allocatedQuantity"},
    {"name": "createdAt", "type": "DateTime", "required": true, "source": "Inventory.createdAt"},
    {"name": "updatedAt", "type": "DateTime", "required": true, "source": "Inventory.updatedAt"}
  ]
}
```

### 5. Request Definition

```json
// ReserveInventoryRequest.json (v1.0)
{
  "name": "ReserveInventoryRequest",
  "version": "1.0",
  "type": "command",
  "commandType": "Process",
  "basis": ["Inventory"],
  "resultType": "InventoryOperationResultDto",
  "parameters": [
    {
      "name": "quantity",
      "type": "PositiveInteger",
      "required": true,
      "description": "Amount to reserve"
    },
    {
      "name": "orderId",
      "type": "UUID",
      "required": true,
      "description": "Order requesting the reservation"
    },
    {
      "name": "notes",
      "type": "string",
      "required": false,
      "constraints": {
        "maxLength": 500
      }
    }
  ]
}
```

### 6. Event Definition

```json
// InventoryReserved.json (v1.0)
{
  "name": "InventoryReserved",
  "version": "1.0",
  "type": "Notify",
  "dataDto": "InventoryOperationResultDto",
  "metadata": [
    {"name": "eventId", "type": "UUID", "required": true},
    {"name": "eventType", "type": "string", "required": true, "const": "InventoryReserved"},
    {"name": "eventVersion", "type": "string", "required": true, "const": "1.0"},
    {"name": "timestamp", "type": "DateTime", "required": true},
    {"name": "correlationId", "type": "UUID", "required": true},
    {"name": "causationId", "type": "UUID", "required": false},
    {"name": "source", "type": "string", "required": true, "const": "inventory-service"}
  ],
  "description": "Published when inventory is successfully reserved for an order"
}
```

### 7. Endpoint Definition

```json
// ReserveInventoryEndpoint.json (v1.0)
{
  "name": "ReserveInventory",
  "version": "1.0",
  "uri": "/api/v1/inventory/{id}/reserve",
  "method": "POST",
  "basis": "InventoryManagementService.ReserveStock",
  "authentication": "ApiKey",
  "parameters": [
    {
      "name": "id",
      "location": "Route",
      "type": "UUID",
      "required": true,
      "description": "Inventory record ID"
    },
    {
      "name": "request",
      "location": "Body",
      "type": "ReserveInventoryRequest",
      "required": true
    }
  ],
  "responses": [
    {
      "status": 200,
      "type": "InventoryOperationResultDto",
      "description": "Reservation successful"
    },
    {
      "status": 400,
      "type": "ErrorDto",
      "description": "Invalid request parameters"
    },
    {
      "status": 404,
      "type": "ErrorDto",
      "description": "Inventory record not found"
    },
    {
      "status": 409,
      "type": "ErrorDto",
      "description": "Insufficient available quantity"
    },
    {
      "status": 401,
      "type": "ErrorDto",
      "description": "Unauthorized"
    },
    {
      "status": 500,
      "type": "ErrorDto",
      "description": "Internal server error"
    }
  ]
}
```

### 8. Implementation Flow

**When a client calls POST /api/v1/inventory/{id}/reserve**:

1. **API Layer** validates request against `ReserveInventoryEndpoint` contract:
   - Validates `id` parameter is valid UUID (route)
   - Validates request body matches `ReserveInventoryRequest`
   - Returns 400 with validation errors if invalid

2. **Service Layer** processes the reserve operation:
   - Loads Inventory entity (matches `Inventory` contract)
   - Validates business rules (sufficient available quantity)
   - Updates reservedQuantity field
   - Persists changes

3. **Response** returns `InventoryOperationResultDto`:
   - Includes all required entity-prefixed fields (ProductId, WarehouseId, etc.)
   - Includes computed `availableQuantity`
   - Matches `InventoryOperationResultDto` contract

4. **Event Published** to message bus:
   - Creates `InventoryReserved` event
   - Includes standard metadata (eventId, timestamp, correlationId, source)
   - Data payload is `InventoryOperationResultDto`
   - Other services can subscribe and react

5. **Contract Validation** ensures:
   - Request matched `ReserveInventoryRequest` contract ✓
   - Response matches `InventoryOperationResultDto` contract ✓
   - Event matches `InventoryReserved` contract ✓
   - All basis entities (Inventory) were properly affected ✓
   - All identity fields from referenced entities included ✓

This complete chain demonstrates how contracts flow from entity definitions through DTOs, requests, endpoints, and events to provide end-to-end type safety and consistency.

### Event
An Event is a domain/integration message that will be put on a service bus for instance. Events have an Event Type which is Create/Update/Delete/Notify and a data field which is a Dto.

**Event Components**:
- **Name**: Event identifier (e.g., InventoryReserved, OrderCreated)
- **Type**: Event classification (Create/Update/Delete/Notify)
- **Data**: The Dto containing event payload
- **Version**: Event contract version
- **Metadata**: Standard event metadata (eventId, timestamp, correlationId, source)

**Event Types**:
- **Create**: Entity instance was created (includes full entity data)
- **Update**: Entity instance was modified (may include full or partial data)
- **Delete**: Entity instance was removed (includes identity fields)
- **Notify**: State change or process event (varies by use case)

**Versioning**:
- Events are versioned independently from their data DTOs
- Event structure (metadata fields) is separate from data payload version
- Consumers must declare which event versions they support
- Producers may emit multiple event versions during migration

**Event Metadata** (standard for all events):
- **eventId**: UUID - unique identifier for this event instance
- **eventType**: string - the event name/type
- **eventVersion**: string - version of this event contract
- **timestamp**: DateTime - when the event occurred
- **correlationId**: UUID - for tracing related events
- **causationId**: UUID - the event/command that caused this event
- **source**: string - service that emitted the event

**Idempotency and Ordering**:
- Events must be idempotent (safe to process multiple times)
- Include sequence numbers or version fields for ordering
- Consumers responsible for handling duplicates
- No guaranteed ordering across event types

**Example**:
```
Event: InventoryReserved v1.0
  - type: Notify
  - data: InventoryOperationResultDto
  - metadata:
      - eventId: UUID (required)
      - eventType: "InventoryReserved" (required)
      - eventVersion: "1.0" (required)
      - timestamp: DateTime (required)
      - correlationId: UUID (required)
      - causationId: UUID (optional)
      - source: "inventory-service" (required)
  - dataFields (from InventoryOperationResultDto):
      - inventoryId: UUID
      - ProductId: UUID
      - quantity: integer
      - reservedQuantity: integer
      - operation: string ("reserve")

Event: ProductCreated v1.0
  - type: Create
  - data: ProductDto
  - metadata: (same standard fields)
```

### Endpoint
An endpoint represents the definition of an API endpoint. Each endpoint specifies a Name, a URI, an HttpMethod, input params that is an object with a request specifying the Request Claim and a location that will be Route/Query/Body. Each Endpoint also specifies a result Type which is a Dto and also has a Basis which where present should refer to a Service Contract that it fulfils in part or in totality.

**Endpoint Components**:
- **Name**: Endpoint identifier (e.g., GetInventory, ReserveStock)
- **URI**: URL path template with parameters (e.g., `/api/v1/inventory/{id}`)
- **HttpMethod**: GET, POST, PUT, PATCH, DELETE
- **Input Parameters**: Collection of parameters with locations
- **Result Type**: The Dto returned on success
- **Error Responses**: Expected error response types and status codes
- **Basis**: Optional reference to Service Contract operation this fulfills
- **Authentication**: Required authentication method (if any)

**Parameter Locations**:
- **Route**: Path parameters (e.g., `/inventory/{id}` - id is route param)
- **Query**: Query string parameters (e.g., `?page=1&size=20`)
- **Body**: Request body (JSON payload)
- **Header**: HTTP headers (e.g., `X-Correlation-Id`)

**Multiple Parameters**:
- An endpoint may have parameters in different locations
- Route parameters are required by definition
- Query parameters may be optional (specify with required flag)
- Body parameters typically use a Request claim
- Each parameter location is independent

**HTTP Status Codes**:
- **2xx Success**: 200 (OK), 201 (Created), 204 (No Content)
- **4xx Client Errors**: 400 (Bad Request), 401 (Unauthorized), 403 (Forbidden), 404 (Not Found), 409 (Conflict)
- **5xx Server Errors**: 500 (Internal Server Error), 503 (Service Unavailable)

**Error Response Contract**:
- All endpoints must define expected error responses
- Use standard error Dto format across all endpoints
- Include validation error details for 400 responses

**Example**:
```
Endpoint: GetInventoryById
  - name: "GetInventoryById"
  - uri: "/api/v1/inventory/{id}"
  - method: GET
  - basis: InventoryManagementService.GetInventory
  - authentication: ApiKey
  - parameters:
      - id: location=Route, type=UUID, required=true
  - resultType: InventoryItemDto (success)
  - responses:
      - 200: InventoryItemDto
      - 404: ErrorDto (inventory not found)
      - 401: ErrorDto (unauthorized)
      - 500: ErrorDto (server error)

Endpoint: ListInventory
  - name: "ListInventory"
  - uri: "/api/v1/inventory"
  - method: GET
  - basis: InventoryManagementService.ListInventory
  - authentication: ApiKey
  - parameters:
      - warehouseId: location=Query, type=UUID, required=false
      - productId: location=Query, type=UUID, required=false
      - page: location=Query, type=PositiveInteger, required=false, default=1
      - size: location=Query, type=PositiveInteger, required=false, default=20
  - resultType: InventoryListDto
  - responses:
      - 200: InventoryListDto
      - 400: ErrorDto (invalid parameters)
      - 401: ErrorDto (unauthorized)

Endpoint: ReserveInventory
  - name: "ReserveInventory"
  - uri: "/api/v1/inventory/{id}/reserve"
  - method: POST
  - basis: InventoryManagementService.ReserveStock
  - authentication: ApiKey
  - parameters:
      - id: location=Route, type=UUID, required=true
      - request: location=Body, type=ReserveInventoryRequest, required=true
  - resultType: InventoryOperationResultDto
  - responses:
      - 200: InventoryOperationResultDto
      - 400: ErrorDto (invalid request)
      - 404: ErrorDto (inventory not found)
      - 409: ErrorDto (insufficient quantity)
      - 401: ErrorDto (unauthorized)
``` 
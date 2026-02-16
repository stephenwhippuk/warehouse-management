# DI-Enabled Server Example

This example demonstrates a complete HTTP server with **Dependency Injection (DI)**, showing production-ready patterns for service composition, lifecycle management, and controller integration.

## Location

**Source**: `examples/di_server.cpp` (557 lines)  
**Executable**: `build/examples/di-server`

## Features

### Service Lifetimes Demonstrated

1. **Singleton** (created once, shared across all requests):
   - `MockDatabase` - Simulated database connection
   - `SimpleLogger` - Application logger

2. **Scoped** (created per request, destroyed after response):
   - `InventoryRepository` - Data access layer
   - `InventoryService` - Business logic layer

3. **Service Composition** (automatic dependency resolution):
   ```
   InventoryService → InventoryRepository → MockDatabase
                                         → SimpleLogger
   ```

### Controllers with DI

- **HealthController**: Uses singleton services
- **InventoryController**: Demonstrates full CRUD with scoped services
  - GET `/api/v1/inventory` - List all items
  - GET `/api/v1/inventory/{id}` - Get by ID
  - POST `/api/v1/inventory` - Create item
  - POST `/api/v1/inventory/{id}/reserve` - Reserve inventory

## Building

```bash
cd /path/to/warehouse-management/services/cpp/shared/http-framework/build
make di-server
```

## Running

### Start Server
```bash
./examples/di-server
```

### Port Configuration
Default port is **8088**. To change, edit line 493 in `examples/di_server.cpp`:
```cpp
http::HttpHost host(8088, "0.0.0.0"); // Change 8088 to desired port
```

## Testing Endpoints

### Health Check (Singleton Services)
```bash
curl http://localhost:8088/api/health | jq .
```

**Response**:
```json
{
  "database": "connected",
  "status": "healthy",
  "timestamp": 1771202600782715384
}
```

### List Inventory (Scoped Services)
```bash
curl http://localhost:8088/api/v1/inventory | jq .
```

**Response**:
```json
[
  {
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "productId": "prod-001",
    "warehouseId": "wh-001",
    "locationId": "loc-001",
    "quantity": 100,
    "reserved": 0,
    "available": 100
  }
]
```

### Get Item by ID
```bash
curl http://localhost:8088/api/v1/inventory/550e8400-e29b-41d4-a716-446655440000 | jq .
```

### Create New Item
```bash
curl -X POST http://localhost:8088/api/v1/inventory \
     -H 'Content-Type: application/json' \
     -d '{"productId":"550e8400-aaaa-bbbb-cccc-446655440001", "quantity": 50}' | jq .
```

**Response (201 Created)**:
```json
{
  "id": "550e8400-e29b-41d4-a716-446655441003",
  "productId": "550e8400-aaaa-bbbb-cccc-446655440001",
  "warehouseId": "wh-001",
  "locationId": "loc-001",
  "quantity": 50,
  "reserved": 0,
  "available": 50
}
```

### Reserve Inventory
```bash
curl -X POST http://localhost:8088/api/v1/inventory/550e8400-e29b-41d4-a716-446655440000/reserve \
     -H 'Content-Type: application/json' \
     -d '{"quantity": 25}' | jq .
```

**Response**:
```json
{
  "success": true,
  "reserved": 25
}
```

## Observing Service Lifecycle

Watch the server output to see service creation and destruction:

### Server Startup (Singletons Created Once)
```
🏗️  [Step 2] Building service provider...
   → Creating singleton instances:
[MockDatabase] ✅ Created (Singleton)
[SimpleLogger] ✅ Created (Singleton)
```

### Per-Request Lifecycle (Scoped Services)

**Request 1**:
```
📨 Request: GET /api/v1/inventory - Listing all items
[InventoryRepository] ✅ Created (Scoped) - uses DB: Mock Database (In-Memory)
[InventoryService] ✅ Created (Scoped)
✅ Response: 3 items returned
[InventoryService] 🗑️  Destroyed (Scoped)
[InventoryRepository] 🗑️  Destroyed (Scoped) - auto cleanup after request
```

**Request 2** (new instances created):
```
📨 Request: POST /api/v1/inventory - Creating item
[InventoryRepository] ✅ Created (Scoped) - uses DB: Mock Database (In-Memory)
[InventoryService] ✅ Created (Scoped)
✅ Response: Item created with ID 550e8400-e29b-41d4-a716-446655441003
[InventoryService] 🗑️  Destroyed (Scoped)
[InventoryRepository] 🗑️  Destroyed (Scoped) - auto cleanup after request
```

### Server Shutdown (Singletons Destroyed)
```
[SimpleLogger] 🗑️  Destroyed
[MockDatabase] 🗑️  Destroyed
```

## Key Patterns

### 1. Service Interface
```cpp
class IInventoryService {
public:
    virtual ~IInventoryService() = default;
    virtual std::vector<InventoryItem> getAll() const = 0;
    virtual bool reserve(const std::string& id, int quantity) = 0;
};
```

### 2. Constructor Injection
```cpp
class InventoryService : public IInventoryService {
public:
    explicit InventoryService(http::IServiceProvider& provider)
        : repository_(provider.getService<IInventoryRepository>()) {
        // Dependencies automatically resolved
    }
private:
    std::shared_ptr<IInventoryRepository> repository_;
};
```

### 3. Service Registration
```cpp
http::ServiceCollection services;
services.addService<IDatabase, MockDatabase>(http::ServiceLifetime::Singleton);
services.addService<IInventoryService, InventoryService>(http::ServiceLifetime::Scoped);
auto provider = services.buildServiceProvider();
```

### 4. Controller Handler with DI
```cpp
Get("/", [this](http::HttpContext& ctx) {
    // Resolve scoped service from request context
    auto service = ctx.getService<IInventoryService>();
    auto items = service->getAll();
    
    json j = json::array();
    for (const auto& item : items) {
        j.push_back(item.toJson());
    }
    return j.dump();
});
```

### 5. ServiceScopeMiddleware Setup
```cpp
http::HttpHost host(8088, "0.0.0.0");

// CRITICAL: Add ServiceScopeMiddleware first
host.use(std::make_shared<http::ServiceScopeMiddleware>(provider));

// Then add controllers
host.addController(std::make_shared<InventoryController>(*provider));

host.start();
```

## Architecture Layers

```
┌─────────────────────────────────────────────┐
│ Controllers (HTTP)                          │
│  - HealthController                         │
│  - InventoryController                      │
│  → Resolve services from HttpContext        │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│ Services (Business Logic)    [SCOPED]       │
│  - IInventoryService / InventoryService     │
│  → Business rules, validation               │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│ Repositories (Data Access)   [SCOPED]       │
│  - IInventoryRepository / InventoryRepository│
│  → Data queries, persistence                │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│ Infrastructure (Database, Logger) [SINGLETON]│
│  - IDatabase / MockDatabase                 │
│  - ILogger / SimpleLogger                   │
│  → Shared resources, connections            │
└─────────────────────────────────────────────┘
```

## Benefits Demonstrated

1. **Testability**: All services mockable via interfaces
2. **Separation of Concerns**: Clear layer boundaries
3. **Resource Management**: Automatic cleanup with RAII
4. **Dependency Inversion**: High-level modules depend on abstractions
5. **Lifecycle Control**: Explicit singleton vs scoped lifetimes
6. **Type Safety**: Compile-time dependency verification

## Migration to Real Services

To migrate inventory-service to use this pattern:

1. **Create interfaces** for existing services:
   ```cpp
   class IInventoryService { /* ... */ };
   class IInventoryRepository { /* ... */ };
   ```

2. **Update implementations** to use constructor injection:
   ```cpp
   class InventoryService : public IInventoryService {
   public:
       explicit InventoryService(http::IServiceProvider& provider);
   };
   ```

3. **Register services** in Application.cpp:
   ```cpp
   services.addService<IInventoryService, InventoryService>(Scoped);
   ```

4. **Update controllers** to resolve from context:
   ```cpp
   auto service = ctx.getService<IInventoryService>();
   ```

5. **Add middleware** to HttpHost:
   ```cpp
   host.use(std::make_shared<http::ServiceScopeMiddleware>(provider));
   ```

## Troubleshooting

### Error: "Service scope not set"
**Cause**: ServiceScopeMiddleware not added or not added first  
**Fix**: Ensure `host.use(std::make_shared<http::ServiceScopeMiddleware>(provider))` is called before adding controllers

### Error: "Service not found"
**Cause**: Service not registered in ServiceCollection  
**Fix**: Add service registration: `services.addService<IService, Service>(lifetime)`

### Error: "Address already in use"
**Cause**: Another process using port 8088  
**Fix**: Change port in di_server.cpp line 493, rebuild, and restart

## See Also

- **[MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)** - Step-by-step migration from manual instantiation to DI
- **[ORIGINAL_PHASE_5_COMPLETE.md](ORIGINAL_PHASE_5_COMPLETE.md)** - Validation and testing results
- **[DI_AND_PLUGIN_DESIGN.md](DI_AND_PLUGIN_DESIGN.md)** - Architecture and design decisions

---

*Example validated: February 16, 2026*  
*All test cases: 50 | All assertions: 347 | Status: ✅ PASSING*

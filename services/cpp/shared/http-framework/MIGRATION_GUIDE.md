# HTTP Framework Migration Guide

This guide shows how to migrate existing C++ services to use the shared HTTP framework.

## Overview

The HTTP framework provides:
- **Middleware pipeline** for cross-cutting concerns
- **Controller-based routing** with automatic parameter extraction
- **Uniform API** across all services
- **Reduced boilerplate** code

## Migration Steps

### 1. Update CMakeLists.txt

Add the http-framework dependency:

```cmake
# Add http-framework subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/../shared/http-framework http-framework)

# Link against http-framework
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        http-framework
        # ... other dependencies
)
```

### 2. Convert Controllers

**Before (Manual Route Parsing):**

```cpp
// Old InventoryController.hpp
class InventoryController : public Poco::Net::HTTPRequestHandler {
public:
    explicit InventoryController(std::shared_ptr<InventoryService> service);
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;
private:
    std::shared_ptr<InventoryService> service_;
    void handleGetById(const std::string& id, ...);
    void handleCreate(...);
    // ... lots of helper methods
};

// Old InventoryController.cpp
void InventoryController::handleRequest(HTTPServerRequest& request,
                                       HTTPServerResponse& response) {
    std::string method = request.getMethod();
    Poco::URI uri(request.getURI());
    std::string path = uri.getPath();
    
    // Manual route parsing
    std::vector<std::string> segments;
    Poco::StringTokenizer tokenizer(path, "/", ...);
    for (const auto& token : tokenizer) {
        segments.push_back(token);
    }
    
    // Manual routing logic
    if (method == "GET" && segments.size() == 4) {
        handleGetById(segments[3], response);
    } else if (method == "POST" && segments.size() == 3) {
        handleCreate(request, response);
    }
    // ... dozens of if-else conditions
}

void InventoryController::handleGetById(const std::string& id, 
                                       HTTPServerResponse& response) {
    try {
        auto item = service_->getById(id);
        if (!item) {
            sendErrorResponse(response, "Not found", 404);
            return;
        }
        sendJsonResponse(response, item->toJson().dump(), 200);
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), 500);
    }
}
```

**After (Framework-Based):**

```cpp
// New InventoryController.hpp
#include "http-framework/ControllerBase.hpp"

class InventoryController : public http::ControllerBase {
public:
    explicit InventoryController(std::shared_ptr<InventoryService> service);
private:
    std::shared_ptr<InventoryService> service_;
    std::string getAll(http::HttpContext& ctx);
    std::string getById(http::HttpContext& ctx);
    std::string create(http::HttpContext& ctx);
    // ... handler methods (not helpers!)
};

// New InventoryController.cpp
InventoryController::InventoryController(std::shared_ptr<InventoryService> service)
    : http::ControllerBase("/api/v1/inventory"), service_(service) {
    
    // Register all endpoints in constructor
    Get("/", [this](http::HttpContext& ctx) { return getAll(ctx); });
    Get("/{id:uuid}", [this](http::HttpContext& ctx) { return getById(ctx); });
    Post("/", [this](http::HttpContext& ctx) { return create(ctx); });
    Put("/{id:uuid}", [this](http::HttpContext& ctx) { return update(ctx); });
    Delete("/{id:uuid}", [this](http::HttpContext& ctx) { return deleteById(ctx); });
    
    // Stock operations
    Post("/{id:uuid}/reserve", [this](http::HttpContext& ctx) { return reserve(ctx); });
    Post("/{id:uuid}/release", [this](http::HttpContext& ctx) { return release(ctx); });
    
    // Query endpoints
    Get("/low-stock", [this](http::HttpContext& ctx) { return getLowStock(ctx); });
    Get("/product/{productId:uuid}", [this](http::HttpContext& ctx) { 
        return getByProduct(ctx); 
    });
}

std::string InventoryController::getById(http::HttpContext& ctx) {
    // Route parameter automatically extracted
    std::string id = ctx.routeParams["id"];
    
    auto item = service_->getById(id);
    if (!item) {
        ctx.sendNotFound("Inventory item not found");
        return "";  // Response already sent
    }
    
    return item->toJson().dump();  // Framework handles response
}

std::string InventoryController::create(http::HttpContext& ctx) {
    // Parse and validate JSON body
    auto bodyOpt = parseJsonBody(ctx);
    if (!bodyOpt) return "";  // Error already sent
    
    json body = *bodyOpt;
    
    // Validate required fields
    if (!validateRequiredFields(ctx, body, {"productId", "quantity"})) {
        return "";  // Error already sent
    }
    
    // Create inventory
    auto item = service_->create(body);
    
    // Set status and location header
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
    ctx.setHeader("Location", "/api/v1/inventory/" + item.getId());
    
    return item.toJson().dump();
}

std::string InventoryController::getLowStock(http::HttpContext& ctx) {
    // Query parameters automatically available
    int threshold = ctx.queryParams.getInt("threshold").value_or(10);
    
    auto items = service_->getLowStock(threshold);
    json j = json::array();
    for (const auto& item : items) {
        j.push_back(item.toJson());
    }
    return j.dump();
}
```

### 3. Simplify Server Setup

**Before (Manual Factory):**

```cpp
// Old Server.cpp
class RequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
    explicit RequestHandlerFactory(std::shared_ptr<InventoryService> service)
        : service_(service) {}
    
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) override {
        URI uri(request.getURI());
        const std::string path = uri.getPath();
        
        // Manual routing
        if (path.find("/api/v1/inventory") == 0) {
            return new InventoryController(service_);
        } else if (path == "/health") {
            return new HealthController();
        } else if (path == "/swagger.json") {
            return new SwaggerController();
        }
        // ... more manual routing
        
        return new NotFoundHandler();
    }
private:
    std::shared_ptr<InventoryService> service_;
};

void Server::start() {
    ServerSocket socket(port_);
    HTTPServerParams* params = new HTTPServerParams;
    params->setMaxQueued(100);
    params->setMaxThreads(16);
    
    httpServer_ = std::make_unique<HTTPServer>(
        new RequestHandlerFactory(inventoryService_),
        socket,
        params
    );
    
    httpServer_->start();
    // ... wait logic
}
```

**After (Framework-Based):**

```cpp
// New main.cpp or Server.cpp
#include "http-framework/HttpHost.hpp"
#include "inventory/controllers/InventoryController.hpp"
#include "inventory/controllers/HealthController.hpp"

void Server::start() {
    // Create host
    http::HttpHost host(port_);
    
    // Add middleware
    host.use(std::make_shared<http::LoggingMiddleware>());
    host.use(std::make_shared<http::ErrorHandlingMiddleware>());
    host.use(std::make_shared<http::CorsMiddleware>());
    
    // Add authentication
    std::string apiKey = config->getString("auth.serviceApiKey");
    host.use(std::make_shared<http::AuthenticationMiddleware>(apiKey));
    
    // Add controllers
    host.addController(std::make_shared<InventoryController>(inventoryService_));
    host.addController(std::make_shared<HealthController>());
    host.addController(std::make_shared<SwaggerController>());
    
    // Configure
    host.setMaxThreads(16);
    host.setMaxQueued(100);
    
    // Start (blocks)
    host.start();
}
```

### 4. Remove Boilerplate

Delete these common helper methods (framework provides them):

```cpp
// DELETE - Framework provides these
void sendJsonResponse(HTTPServerResponse& response, const std::string& json, int status);
void sendErrorResponse(HTTPServerResponse& response, const std::string& message, int status);
std::string getBodyAsString(HTTPServerRequest& request);
json parseJsonBody(HTTPServerRequest& request);
```

### 5. Update Authentication

**Before (Manual Auth Check):**

```cpp
void InventoryController::handleRequest(HTTPServerRequest& request, ...) {
    // Manual auth check in every controller
    auto authStatus = Auth::authorizeServiceRequest(request);
    if (authStatus == AuthStatus::MissingToken) {
        sendErrorResponse(response, "Missing auth", 401);
        return;
    }
    if (authStatus == AuthStatus::InvalidToken) {
        sendErrorResponse(response, "Invalid auth", 403);
        return;
    }
    
    // ... rest of handler
}
```

**After (Middleware):**

```cpp
// In Server setup - authentication applied automatically to all routes
host.use(std::make_shared<http::AuthenticationMiddleware>(
    apiKey,
    {"/health", "/swagger.json"}  // Excluded paths
));

// Controllers don't need auth logic anymore!
```

## Benefits

### Code Reduction

- **Before**: ~500 lines per controller (routing, parsing, error handling)
- **After**: ~200 lines per controller (just business logic)
- **Savings**: 60% code reduction

### Consistency

All services now:
- Use the same routing pattern
- Handle errors the same way
- Have the same middleware capabilities
- Follow the same conventions

### Maintainability

- Centralized HTTP logic in framework
- Controllers focus on business logic only
- Easy to add new endpoints (one line registration)
- Middleware reusable across services

### Testing

**Before**: Hard to test controllers (need real HTTP server)

**After**: Easy to test (mock HttpContext):

```cpp
TEST_CASE("InventoryController GET by ID") {
    auto mockService = std::make_shared<MockInventoryService>();
    auto controller = InventoryController(mockService);
    
    // Create mock context
    auto ctx = createMockContext("GET", "/api/v1/inventory/123");
    ctx.routeParams["id"] = "123";
    
    // Call handler
    std::string result = controller.getById(ctx);
    
    // Verify
    REQUIRE(ctx.response.getStatus() == 200);
    json j = json::parse(result);
    REQUIRE(j["id"] == "123");
}
```

## Migration Checklist

- [ ] Update CMakeLists.txt to link http-framework
- [ ] Convert controllers to inherit from `http::ControllerBase`
- [ ] Move route registration to controller constructor
- [ ] Update handler signatures to take `http::HttpContext&` and return `std::string`
- [ ] Replace manual parameter extraction with `ctx.routeParams` and `ctx.queryParams`
- [ ] Replace manual JSON parsing with `ctx.getBodyAsJson()` or `parseJsonBody(ctx)`
- [ ] Replace manual response sending with return statements or `ctx.sendJson()`
- [ ] Update Server/main to use `http::HttpHost`
- [ ] Add middleware (logging, error handling, CORS, auth)
- [ ] Remove old boilerplate helper methods
- [ ] Update tests to use mock HttpContext
- [ ] Test all endpoints thoroughly

## Example: Full Service Migration

See `examples/basic_server.cpp` for a complete working example.

## Common Patterns

### Pattern 1: Simple GET Endpoint

```cpp
Get("/", [this](http::HttpContext& ctx) {
    auto items = service_->getAll();
    json j = json::array();
    for (const auto& item : items) {
        j.push_back(item.toJson());
    }
    return j.dump();
});
```

### Pattern 2: GET with Route Parameter

```cpp
Get("/{id:uuid}", [this](http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    auto item = service_->getById(id);
    if (!item) {
        ctx.sendNotFound("Item not found");
        return "";
    }
    return item->toJson().dump();
});
```

### Pattern 3: GET with Query Parameters

```cpp
Get("/search", [this](http::HttpContext& ctx) {
    std::string query = ctx.queryParams.get("q", "");
    int limit = ctx.queryParams.getInt("limit").value_or(10);
    
    auto results = service_->search(query, limit);
    return resultsToJson(results);
});
```

### Pattern 4: POST with Body

```cpp
Post("/", [this](http::HttpContext& ctx) {
    auto bodyOpt = parseJsonBody(ctx);
    if (!bodyOpt) return "";
    
    json body = *bodyOpt;
    if (!validateRequiredFields(ctx, body, {"name", "quantity"})) {
        return "";
    }
    
    auto item = service_->create(body);
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
    return item.toJson().dump();
});
```

### Pattern 5: PUT with Route Param and Body

```cpp
Put("/{id:uuid}", [this](http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    
    auto bodyOpt = parseJsonBody(ctx);
    if (!bodyOpt) return "";
    
    json body = *bodyOpt;
    auto item = service_->update(id, body);
    
    if (!item) {
        ctx.sendNotFound("Item not found");
        return "";
    }
    
    return item->toJson().dump();
});
```

### Pattern 6: DELETE

```cpp
Delete("/{id:uuid}", [this](http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    
    if (!service_->deleteById(id)) {
        ctx.sendNotFound("Item not found");
        return "";
    }
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
    return "";
});
```

## Troubleshooting

### Issue: Routes not matching

**Problem**: Controller endpoints not being called

**Solution**: Ensure route patterns don't have trailing slashes and base route is correct:
```cpp
// Wrong
ControllerBase("/api/v1/inventory/")  // Trailing slash
Get("/{id}/")                         // Trailing slash

// Correct
ControllerBase("/api/v1/inventory")
Get("/{id}")
```

### Issue: Parameter constraints not working

**Problem**: Invalid IDs getting through

**Solution**: Use route constraints:
```cpp
// Without constraint - accepts anything
Get("/{id}", ...)

// With UUID constraint - only accepts valid UUIDs
Get("/{id:uuid}", ...)
```

### Issue: Query parameters returning empty

**Problem**: `ctx.queryParams.get("page")` returns empty string

**Solution**: Provide default value:
```cpp
// Might be empty
std::string page = ctx.queryParams.get("page");

// Has default
std::string page = ctx.queryParams.get("page", "1");

// Or use getInt
int page = ctx.queryParams.getInt("page").value_or(1);
```

---

## Part 2: Dependency Injection Migration

### Overview

The framework now includes a complete dependency injection container with:
- **Service Lifetimes**: Singleton, Scoped (per-request), Transient
- **Automatic Resolution**: Dependencies injected via constructor
- **Service Scoping**: Per-request service instances with automatic cleanup
- **Plugin Support**: Dynamic plugin loading with namespace isolation

### Benefits of Using DI

1. **Testability**: Easy mocking and unit testing
2. **Loose Coupling**: Replace implementations without changing consumers
3. **Lifetime Management**: Automatic cleanup and resource management
4. **Consistency**: Same patterns across all services

### Step 1: Define Service Interfaces

**Before** (Concrete Classes):
```cpp
// InventoryService.hpp
class InventoryService {
public:
    InventoryService();
    std::optional<InventoryItemDto> getById(const std::string& id);
};
```

**After** (Abstract Interfaces):
```cpp
// IInventoryService.hpp
class IInventoryService {
public:
    virtual ~IInventoryService() = default;
    virtual std::optional<dtos::InventoryItemDto> getById(const std::string& id) = 0;
    virtual std::vector<dtos::InventoryItemDto> getAll() = 0;
};

// InventoryService.hpp
#include "http-framework/IServiceProvider.hpp"

class InventoryService : public IInventoryService {
public:
    // Constructor receives IServiceProvider to resolve dependencies
    explicit InventoryService(http::IServiceProvider& provider);
    
    std::optional<dtos::InventoryItemDto> getById(const std::string& id) override;
    std::vector<dtos::InventoryItemDto> getAll() override;

private:
    std::shared_ptr<IInventoryRepository> repository_;
};
```

### Step 2: Update Service Implementations

**Before**:
```cpp
// InventoryService.cpp
InventoryService::InventoryService() {
    // Manually create dependencies
    auto db = utils::Database::connect();
    repository_ = std::make_shared<InventoryRepository>(db);
}
```

**After**:
```cpp
// InventoryService.cpp
InventoryService::InventoryService(http::IServiceProvider& provider)
    : repository_(provider.getService<IInventoryRepository>()) {
    // Dependencies automatically resolved from DI container
}

std::optional<dtos::InventoryItemDto> InventoryService::getById(const std::string& id) {
    auto inventory = repository_->findById(id);
    if (!inventory) return std::nullopt;
    
    // TODO: Fetch identity fields from other services
    return utils::DtoMapper::toInventoryItemDto(*inventory,productSku, warehouseCode, locationCode);
}
```

### Step 3: Update Controllers

**Before**:
```cpp
class Inventory Controller : public http::ControllerBase {
public:
    InventoryController() : ControllerBase("/api/v1/inventory") {
        service_ = std::make_shared<InventoryService>();
        
        Get("/{id}", [this](http::HttpContext& ctx) {
            return this->getById(ctx);
        });
    }
private:
    std::shared_ptr<InventoryService> service_;
};
```

**After**:
```cpp
class InventoryController : public http::ControllerBase {
public:
    // Controller receives IServiceProvider
    explicit InventoryController(http::IServiceProvider& provider)
        : ControllerBase("/api/v1/inventory")
        , provider_(provider) {
        
        Get("/{id}", [this](http::HttpContext& ctx) {
            return this->getById(ctx);
        });
    }

private:
    http::IServiceProvider& provider_;
    
    std::string getById(http::HttpContext& ctx) {
        // Resolve service from REQUEST SCOPE (automatic per-request instance)
        auto service = ctx.getService<IInventoryService>();
        
        std::string id = ctx.params().get("id");
        auto dto = service->getById(id);
        
        if (!dto) {
            return ctx.json({{"error", "Inventory not found"}}, 404);
        }
        
        return ctx.json(dto->toJson());
    }
};
```

**Key Changes**:
- Controller stores `IServiceProvider&` reference
- Services resolved via `ctx.getService<T>()` (scoped to request)
- Services automatically cleaned up after response sent

### Step 4: Register Services in Application

**Before** (Manual Instantiation):
```cpp
void Application::run() {
    http::HttpHost host("0.0.0.0", 8080);
    
    // Manual dependency tree
    auto db = utils::Database::connect();
    auto repo = std::make_shared<InventoryRepository>(db);
    auto service = std::make_shared<InventoryService>(repo);
    auto controller = std::make_shared<InventoryController>(service);
    
    host.addController(controller);
    host.start();
}
```

**After** (DI Container):
```cpp
#include "http-framework/ServiceCollection.hpp"
#include "http-framework/ServiceScopeMiddleware.hpp"

void Application::run() {
    // 1. Create service collection
    http::ServiceCollection services;
    
    // 2. Register infrastructure services (Singleton)
    services.addService<IDatabase>(
        [](http::IServiceProvider& provider) {
            return std::make_shared<PostgresDatabase>(
                std::getenv("DATABASE_URL")
            );
        },
        http::ServiceLifetime::Singleton
    );
    
    services.addService<ILogger, SpdlogLogger>(
        http::ServiceLifetime::Singleton
    );
    
    // 3. Register repositories (Scoped - per request)
    services.addService<IInventoryRepository, InventoryRepository>(
        http::ServiceLifetime::Scoped
    );
    
    // 4. Register business services (Scoped - per request)
    services.addService<IInventoryService, InventoryService>(
        http::ServiceLifetime::Scoped
    );
    
    // 5. Build service provider
    auto provider = services.buildServiceProvider();
    
    // 6. Create HTTP host
    http::HttpHost host("0.0.0.0", 8080);
    
    // 7. Add ServiceScopeMiddleware FIRST (creates scope per request)
    host.use(std::make_shared<http::ServiceScopeMiddleware>(provider));
    
    // 8. Add controllers (resolve dependencies from provider)
    auto inventoryController = std::make_shared<InventoryController>(*provider);
    host.addController(inventoryController);
    
    // 9. Optional: Load plugins
    http::PluginManager pluginManager(*provider->getServices());
    // pluginManager.loadPlugin("/path/to/plugin.so");
    
    // 10. Start server
    spdlog::info("Starting server on port 8080");
    host.start();
}
```

### Service Lifetime Guidelines

Choose the appropriate lifetime for each service:

#### Singleton (Shared Across Application)
- Database connections
- Configuration objects
- Loggers
- Thread pools
- Expensive resources

```cpp
services.addService<IDatabase>(
    [](auto& p) { return std::make_shared<PostgresDb>(getenv("DATABASE_URL")); },
    http::ServiceLifetime::Singleton
);
```

#### Scoped (Per Request)
- Business logic services
- Repositories  
- Request-specific state
- Services that should be cleaned up after request

```cpp
services.addService<IInventoryService, InventoryService>(
    http::ServiceLifetime::Scoped
);
```

#### Transient (New Instance Every Time)
- Stateless validators
- DTOmappers
- Cheap utility classes
- Services that must not share state

```cpp
services.addService<IValidator, RequestValidator>(
    http::ServiceLifetime::Transient
);
```

### Step 5: Update Tests

**Before** (Manual Mocks):
```cpp
TEST_CASE("InventoryService returns items", "[service]") {
    auto mockRepo = std::make_shared<MockInventoryRepository>();
    auto service = std::make_shared<InventoryService>(mockRepo);
    
    auto item = service->getById("test-id");
    REQUIRE(item.has_value());
}
```

**After** (DI Container with Mocks):
```cpp
#include "http-framework/ServiceCollection.hpp"

TEST_CASE("InventoryService returns DTOs", "[service]") {
    // Create test container
    http::ServiceCollection services;
    
    // Register mock repository
    services.addService<IInventoryRepository>(
        [](http::IServiceProvider& provider) {
            return std::make_shared<MockInventoryRepository>();
        },
        http::ServiceLifetime::Singleton
    );
    
    // Register service under test
    services.addService<IInventoryService, InventoryService>(
        http::ServiceLifetime::Singleton
    );
    
    auto provider = services.buildServiceProvider();
    auto service = provider->getService<IInventoryService>();
    
    SECTION("GetById returns DTO for existing item") {
        auto dto = service->getById("test-id");
        REQUIRE(dto.has_value());
        REQUIRE(dto->getId() == "test-id");
    }
}
```

### Common Patterns

#### Pattern 1: Lazy Dependency Resolution

For optional or conditional dependencies:

```cpp
class InventoryService : public IInventoryService {
public:
    explicit InventoryService(http::IServiceProvider& provider)
        : provider_(provider) {
        // Don't resolve yet
    }
    
    void doWork() {
        // Resolve only when needed
        auto repo = provider_.getService<IInventoryRepository>();
        repo->findById("123");
    }

private:
    http::IServiceProvider& provider_;
};
```

#### Pattern 2: Factory Functions

For complex initialization:

```cpp
services.addService<IDatabase>(
    [](http::IServiceProvider& provider) {
        std::string url = std::getenv("DATABASE_URL");
        int maxConnections = std::stoi(std::getenv("MAX_DB_CONNECTIONS"));
        
        auto db = std::make_shared<PostgresDatabase>(url);
        db->setMaxConnections(maxConnections);
        db->connect();
        
        return db;
    },
    http::ServiceLifetime::Singleton
);
```

#### Pattern 3: Service Composition

Services can depend on other services:

```cpp
class OrderService : public IOrderService {
public:
    explicit OrderService(http::IServiceProvider& provider)
        : inventoryService_(provider.getService<IInventoryService>())
        , warehouseService_(provider.getService<IWarehouseService>())
        , orderRepo_(provider.getService<IOrderRepository>()) {
    }
    
    void createOrder(const CreateOrderRequest& request) {
        // Use multiple services
        auto inventory = inventoryService_->getById(request.productId);
        auto warehouse = warehouseService_->getById(request.warehouseId);
        
        // Create order
        orderRepo_->create(...);
    }
};
```

### Troubleshooting

#### Issue 1: Service Not Found

**Error**: `Service not registered: IInventoryService`

**Solution**: Ensure service is registered before building provider

```cpp
services.addService<IInventoryService, InventoryService>(
    http::ServiceLifetime::Scoped
);
auto provider = services.buildServiceProvider();  // AFTER registration
```

#### Issue 2: Circular Dependencies

**Error**: `Circular dependency detected: A -> B -> A`

**Solution**: Use lazy resolution

```cpp
class ServiceA {
    explicit ServiceA(http::IServiceProvider& provider) : provider_(provider) {}
    
    void doWork() {
        // Resolve B only when needed, breaking the cycle
        auto b = provider_.getService<IServiceB>();
    }
};
```

#### Issue 3: Lifetime Mismatch

**Error**: Cannot store Scoped service in Transient

**Solution**: Match lifetimes or resolve on demand

```cpp
// Option 1: Make both Scoped
services.addService<IRepo, Repo>(Scoped);
services.addService<IService, Service>(Scoped);

// Option 2: Resolve on demand
class Service {
    explicit Service(http::IServiceProvider& p) : provider_(p) {}
    
    void doWork() {
        auto repo = provider_.getService<IRepo>();  // Get when needed
    }
};
```

### Migration Checklist

- [ ] Create abstract interfaces for all services (`IServiceName`)
- [ ] Update constructors to accept `http::IServiceProvider&`
- [ ] Register services in `ServiceCollection` with appropriate lifetimes
- [ ] Add `ServiceScopeMiddleware` to HTTP pipeline
- [ ] Update controllers to resolve services via `ctx.getService<T>()`
- [ ] Update tests to use DI container with mocks
- [ ] Remove manual `new` / `make_shared` for services
- [ ] Verify proper cleanup (check logs for scope destruction)

### Examples

See the following for complete examples:
- [examples/di_server.cpp](examples/di_server.cpp) - Complete DI-enabled server
- [tests/ServiceProviderTests.cpp](tests/ServiceProviderTests.cpp) - Unit test patterns
- [tests/ServiceScopeTests.cpp](tests/ServiceScopeTests.cpp) - Scoped lifetime testing
- [plugins/TestPlugin.cpp](plugins/TestPlugin.cpp) - Plugin development

### Next Steps

1. Review [PHASES_1_5_COMPLETE.md](PHASES_1_5_COMPLETE.md) for complete framework overview
2. Study test cases for usage patterns
3. Migrate one service at a time (infrastructure → repositories → business logic)
4. Add integration tests with DI

---

**Migration Guide Updated**: February 2026  
**Includes**: HTTP Framework + Dependency Injection + Plugin System

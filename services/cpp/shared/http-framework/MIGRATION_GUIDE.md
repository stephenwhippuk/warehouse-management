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

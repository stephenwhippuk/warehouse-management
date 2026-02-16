# HTTP Framework - Shared C++ Library

A modern HTTP framework for C++ services inspired by ASP.NET Core, providing middleware support, controller-based routing, and endpoint registration.

## Features

- **Middleware Pipeline**: Composable middleware chain (authentication, logging, CORS, etc.)
- **Controller Base Class**: Base class with fluent route registration
- **Endpoint Routing**: Route parameters and query parameters with automatic extraction
- **Parameter Dictionary**: Handlers receive unified parameter dictionary
- **JSON Responses**: Standardized JSON response handling
- **Type Safety**: Compile-time route validation where possible

## Architecture

```
HttpHost
  ├── MiddlewarePipeline
  │     ├── Middleware 1 (e.g., Logging)
  │     ├── Middleware 2 (e.g., Authentication)
  │     └── Middleware N (e.g., CORS)
  ├── Router
  │     ├── Route Pattern Matching
  │     └── Parameter Extraction
  └── Controllers
        ├── Controller 1 (base route: /api/v1/inventory)
        │     ├── Endpoint 1 (GET /)
        │     ├── Endpoint 2 (GET /{id})
        │     └── Endpoint 3 (POST /{id}/reserve)
        └── Controller 2 (base route: /api/v1/products)
```

## Quick Start

### 1. Create a Controller

```cpp
#include "http-framework/ControllerBase.hpp"

class InventoryController : public http::ControllerBase {
public:
    InventoryController(std::shared_ptr<InventoryService> service) 
        : ControllerBase("/api/v1/inventory"), service_(service) {
        
        // Register endpoints
        Get("/", [this](http::HttpContext& ctx) {
            return this->getAll(ctx);
        });
        
        Get("/{id}", [this](http::HttpContext& ctx) {
            return this->getById(ctx);
        });
        
        Post("/{id}/reserve", [this](http::HttpContext& ctx) {
            return this->reserve(ctx);
        });
        
        Get("/low-stock", [this](http::HttpContext& ctx) {
            return this->getLowStock(ctx);
        });
    }
    
private:
    std::shared_ptr<InventoryService> service_;
    
    std::string getAll(http::HttpContext& ctx) {
        auto items = service_->getAll();
        json j = json::array();
        for (const auto& item : items) {
            j.push_back(item.toJson());
        }
        return j.dump();
    }
    
    std::string getById(http::HttpContext& ctx) {
        std::string id = ctx.routeParams["id"];
        auto item = service_->getById(id);
        if (!item) {
            ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            return R"({"error": "Not found"})";
        }
        return item->toJson().dump();
    }
    
    std::string reserve(http::HttpContext& ctx) {
        std::string id = ctx.routeParams["id"];
        json body = json::parse(ctx.getBodyAsString());
        int quantity = body["quantity"];
        
        auto result = service_->reserve(id, quantity);
        return result.toJson().dump();
    }
    
    std::string getLowStock(http::HttpContext& ctx) {
        int threshold = std::stoi(ctx.queryParams.get("threshold", "10"));
        auto items = service_->getLowStock(threshold);
        json j = json::array();
        for (const auto& item : items) {
            j.push_back(item.toJson());
        }
        return j.dump();
    }
};
```

### 2. Create Middleware

```cpp
#include "http-framework/Middleware.hpp"

class LoggingMiddleware : public http::Middleware {
public:
    void process(http::HttpContext& ctx, std::function<void()> next) override {
        Logger::info("Request: {} {}", ctx.request.getMethod(), ctx.request.getURI());
        
        auto start = std::chrono::steady_clock::now();
        next(); // Call next middleware
        auto end = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        Logger::info("Response: {} ({}ms)", ctx.response.getStatus(), duration.count());
    }
};
```

### 3. Configure and Start Host

```cpp
#include "http-framework/HttpHost.hpp"
#include "http-framework/ServiceCollection.hpp"
#include "http-framework/ExceptionFilter.hpp"

int main() {
    http::ServiceCollection services;
    auto provider = services.buildServiceProvider();
    auto host = http::HttpHost(8080, provider);
    
    // HttpHost adds ServiceScopeMiddleware + ErrorHandlingMiddleware by default
    // Optional: override default exception filter
    // host.setExceptionFilter(std::make_shared<CustomExceptionFilter>());
    // Add additional middleware
    host.use(std::make_shared<LoggingMiddleware>());
    host.use(std::make_shared<AuthenticationMiddleware>());
    host.use(std::make_shared<CorsMiddleware>());
    
    // Register controllers
    auto inventoryService = std::make_shared<InventoryService>(...);
    host.addController(std::make_shared<InventoryController>(inventoryService));
    
    auto productService = std::make_shared<ProductService>(...);
    host.addController(std::make_shared<ProductController>(productService));
    
    // Start server
    host.start();
    
    return 0;
}
```

## HttpContext

The `HttpContext` provides access to request/response and extracted parameters:

```cpp
struct HttpContext {
    Poco::Net::HTTPServerRequest& request;       // Original request
    Poco::Net::HTTPServerResponse& response;     // Original response
    std::map<std::string, std::string> routeParams;  // Route parameters
    QueryParams queryParams;                     // Query parameters
    std::map<std::string, std::any> items;       // Request-scoped data
    
    std::string getBodyAsString();               // Read request body
    json getBodyAsJson();                        // Parse body as JSON
};
```

## Route Parameters

Route parameters are extracted automatically:

```cpp
// Route: GET /api/v1/inventory/{id}/location/{locationId}
Get("/{id}/location/{locationId}", [](HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    std::string locationId = ctx.routeParams["locationId"];
    // ...
});
```

## Query Parameters

Query parameters are accessed via the `queryParams` helper:

```cpp
// URL: /api/v1/inventory?page=2&limit=50
Get("/", [](HttpContext& ctx) {
    int page = std::stoi(ctx.queryParams.get("page", "1"));
    int limit = std::stoi(ctx.queryParams.get("limit", "10"));
    // ...
});
```

## Built-in Middleware

- **LoggingMiddleware**: Request/response logging
- **AuthenticationMiddleware**: Service-to-service auth
- **CorsMiddleware**: Cross-origin resource sharing
- **ErrorHandlingMiddleware**: Centralized error handling

## Response Helpers

The framework provides helpers for common response scenarios:

```cpp
// Success with DTO
ctx.sendJson(item.toJson());

// Not found
ctx.sendNotFound("Resource not found");

// Error
ctx.sendError("Invalid input", Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);

// Created
ctx.sendCreated("/api/v1/inventory/123", item.toJson());
```

## Advanced Features

### Request-Scoped Data

Share data between middleware and controllers:

```cpp
// In middleware:
ctx.items["userId"] = userId;

// In controller:
std::string userId = std::any_cast<std::string>(ctx.items["userId"]);
```

### Custom Route Constraints

```cpp
Get("/{id:uuid}", [](HttpContext& ctx) {
    // id is validated as UUID
});

Get("/page/{page:int}", [](HttpContext& ctx) {
    // page is validated as integer
});
```

## Building

Add to your service's CMakeLists.txt:

```cmake
# Link http-framework
add_subdirectory(${CMAKE_SOURCE_DIR}/../shared/http-framework http-framework)
target_link_libraries(${PROJECT_NAME} http-framework)
```

## Testing

Unit test your controllers without starting a server:

```cpp
TEST_CASE("InventoryController GET by ID") {
    auto ctx = createMockContext("GET", "/api/v1/inventory/123");
    auto controller = InventoryController(mockService);
    
    std::string result = controller.handle(ctx);
    
    REQUIRE(ctx.response.getStatus() == 200);
    REQUIRE_FALSE(result.empty());
}
```

# HTTP Framework - Quick Reference

## Setup

```cpp
#include "http-framework/HttpHost.hpp"
#include "http-framework/ControllerBase.hpp"
#include "http-framework/Middleware.hpp"

// Create host
http::HttpHost host(8080);

// Add middleware
host.use(std::make_shared<http::LoggingMiddleware>());
host.use(std::make_shared<http::ErrorHandlingMiddleware>());
host.use(std::make_shared<http::CorsMiddleware>());
host.use(std::make_shared<http::AuthenticationMiddleware>(apiKey));

// Add controllers
host.addController(std::make_shared<MyController>(service));

// Configure
host.setMaxThreads(16);
host.setMaxQueued(100);

// Start (blocks)
host.start();
```

## Controller Definition

```cpp
class MyController : public http::ControllerBase {
public:
    MyController(std::shared_ptr<MyService> service)
        : http::ControllerBase("/api/v1/resource"), service_(service) {
        
        // GET endpoints
        Get("/", [this](http::HttpContext& ctx) { 
            return getAll(ctx); 
        });
        
        Get("/{id:uuid}", [this](http::HttpContext& ctx) { 
            return getById(ctx); 
        });
        
        Get("/search", [this](http::HttpContext& ctx) { 
            return search(ctx); 
        });
        
        // POST endpoints
        Post("/", [this](http::HttpContext& ctx) { 
            return create(ctx); 
        });
        
        // PUT endpoints
        Put("/{id:uuid}", [this](http::HttpContext& ctx) { 
            return update(ctx); 
        });
        
        // DELETE endpoints
        Delete("/{id:uuid}", [this](http::HttpContext& ctx) { 
            return deleteById(ctx); 
        });
        
        // Custom endpoints
        Post("/{id:uuid}/action", [this](http::HttpContext& ctx) { 
            return customAction(ctx); 
        });
    }

private:
    std::shared_ptr<MyService> service_;
    
    std::string getAll(http::HttpContext& ctx);
    std::string getById(http::HttpContext& ctx);
    // ...
};
```

## Handler Patterns

### GET All Items
```cpp
std::string getAll(http::HttpContext& ctx) {
    auto items = service_->getAll();
    json j = json::array();
    for (const auto& item : items) {
        j.push_back(item.toJson());
    }
    return j.dump();
}
```

### GET by ID
```cpp
std::string getById(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    auto item = service_->getById(id);
    
    if (!item) {
        ctx.sendNotFound("Item not found");
        return "";
    }
    
    return item->toJson().dump();
}
```

### GET with Query Parameters
```cpp
std::string search(http::HttpContext& ctx) {
    std::string query = ctx.queryParams.get("q", "");
    int page = ctx.queryParams.getInt("page").value_or(1);
    int limit = ctx.queryParams.getInt("limit").value_or(10);
    
    auto results = service_->search(query, page, limit);
    return resultsToJson(results);
}
```

### POST (Create)
```cpp
std::string create(http::HttpContext& ctx) {
    auto bodyOpt = parseJsonBody(ctx);
    if (!bodyOpt) return "";
    
    json body = *bodyOpt;
    
    if (!validateRequiredFields(ctx, body, {"name", "quantity"})) {
        return "";
    }
    
    auto item = service_->create(body);
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
    ctx.setHeader("Location", "/api/v1/resource/" + item.getId());
    
    return item.toJson().dump();
}
```

### PUT (Update)
```cpp
std::string update(http::HttpContext& ctx) {
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
}
```

### DELETE
```cpp
std::string deleteById(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    
    if (!service_->deleteById(id)) {
        ctx.sendNotFound("Item not found");
        return "";
    }
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
    return "";
}
```

### POST (Custom Action)
```cpp
std::string reserve(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    
    auto bodyOpt = parseJsonBody(ctx);
    if (!bodyOpt) return "";
    
    json body = *bodyOpt;
    int quantity = body["quantity"];
    
    try {
        auto result = service_->reserve(id, quantity);
        return result.toJson().dump();
    } catch (const std::exception& e) {
        ctx.sendError(e.what(), Poco::Net::HTTPResponse::HTTP_CONFLICT);
        return "";
    }
}
```

## HttpContext Reference

### Route Parameters
```cpp
std::string id = ctx.routeParams["id"];
std::string subId = ctx.routeParams["subId"];
```

### Query Parameters
```cpp
// Get with default
std::string name = ctx.queryParams.get("name", "default");

// Get as int
int page = ctx.queryParams.getInt("page").value_or(1);

// Get as bool
bool active = ctx.queryParams.getBool("active").value_or(true);

// Check existence
if (ctx.queryParams.has("filter")) {
    // ...
}
```

### Request Info
```cpp
std::string method = ctx.getMethod();        // GET, POST, etc.
std::string path = ctx.getPath();            // /api/v1/resource
std::string header = ctx.getHeader("X-Custom", "default");
bool hasHeader = ctx.hasHeader("Authorization");
```

### Request Body
```cpp
// As string
std::string body = ctx.getBodyAsString();

// As JSON
json body = ctx.getBodyAsJson();

// With validation (from ControllerBase)
auto bodyOpt = parseJsonBody(ctx);
if (!bodyOpt) return "";  // Error already sent
json body = *bodyOpt;
```

### Response Sending
```cpp
// JSON response (most common - just return string)
return jsonString;

// Set status code
ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
return jsonString;

// Send JSON directly
ctx.sendJson(jsonObject);
return "";

// Error responses
ctx.sendError("Error message", Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
return "";

// Not found
ctx.sendNotFound("Resource not found");
return "";

// No content
ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
return "";

// Created with location
ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
ctx.setHeader("Location", "/api/v1/resource/123");
return jsonString;
```

### Response Headers
```cpp
ctx.setHeader("X-Custom-Header", "value");
ctx.setHeader("Cache-Control", "no-cache");
ctx.setHeader("Location", "/api/v1/resource/123");
```

### Request-Scoped Data
```cpp
// Store data (for middleware to pass to handler)
ctx.items["userId"] = userId;

// Retrieve data
auto userId = std::any_cast<std::string>(ctx.items["userId"]);
```

## Route Constraints

### Available Constraints
```cpp
Get("/{id}",           handler);  // Any value
Get("/{id:uuid}",      handler);  // UUID format
Get("/{page:int}",     handler);  // Integer only
Get("/{name:alpha}",   handler);  // Letters only
Get("/{code:alphanum}", handler); // Letters and numbers
```

### Multiple Parameters
```cpp
Get("/{resource}/{id:uuid}/sub/{subId:int}", handler);
```

## Built-in Middleware

### Logging
```cpp
host.use(std::make_shared<http::LoggingMiddleware>());
// Logs: [HTTP] GET /api/v1/resource
//       [HTTP] 200 /api/v1/resource (45ms)
```

### Error Handling
```cpp
host.use(std::make_shared<http::ErrorHandlingMiddleware>());
// Catches exceptions and returns error JSON
```

### CORS
```cpp
host.use(std::make_shared<http::CorsMiddleware>(
    "https://example.com",           // Origin (or "*")
    "GET, POST, PUT, DELETE",        // Methods
    "Content-Type, Authorization"    // Headers
));
```

### Authentication
```cpp
host.use(std::make_shared<http::AuthenticationMiddleware>(
    "your-api-key",                  // Expected API key
    {"/health", "/swagger.json"}     // Excluded paths
));
```

## Custom Middleware

```cpp
class MyMiddleware : public http::Middleware {
public:
    void process(http::HttpContext& ctx, std::function<void()> next) override {
        // Before handler
        auto start = std::chrono::steady_clock::now();
        Logger::info("Request: {} {}", ctx.getMethod(), ctx.getPath());
        
        // Call next middleware/handler
        next();
        
        // After handler
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        Logger::info("Duration: {}ms", duration.count());
    }
};

// Use it
host.use(std::make_shared<MyMiddleware>());
```

## ControllerBase Helpers

### Parse JSON Body
```cpp
auto bodyOpt = parseJsonBody(ctx);
if (!bodyOpt) return "";  // Error already sent
json body = *bodyOpt;
```

### Validate Required Fields
```cpp
if (!validateRequiredFields(ctx, body, {"field1", "field2", "field3"})) {
    return "";  // Error already sent
}
```

### Get Route Parameter (with validation)
```cpp
auto idOpt = getRouteParam(ctx, "id");
if (!idOpt) return "";  // Error already sent
std::string id = *idOpt;
```

## HTTP Status Codes

```cpp
Poco::Net::HTTPResponse::HTTP_OK                     // 200
Poco::Net::HTTPResponse::HTTP_CREATED                // 201
Poco::Net::HTTPResponse::HTTP_NO_CONTENT             // 204
Poco::Net::HTTPResponse::HTTP_BAD_REQUEST            // 400
Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED           // 401
Poco::Net::HTTPResponse::HTTP_FORBIDDEN              // 403
Poco::Net::HTTPResponse::HTTP_NOT_FOUND              // 404
Poco::Net::HTTPResponse::HTTP_CONFLICT               // 409
Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR  // 500
```

## CMakeLists.txt Integration

```cmake
# Add http-framework subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/../shared/http-framework http-framework)

# Link against http-framework
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        http-framework
        # other dependencies...
)
```

## Common Patterns

### Return Paginated Results
```cpp
std::string getAll(http::HttpContext& ctx) {
    int page = ctx.queryParams.getInt("page").value_or(1);
    int limit = ctx.queryParams.getInt("limit").value_or(10);
    
    auto items = service_->getAll(page, limit);
    int total = service_->count();
    
    json response = {
        {"items", json::array()},
        {"page", page},
        {"limit", limit},
        {"total", total},
        {"pages", (total + limit - 1) / limit}
    };
    
    for (const auto& item : items) {
        response["items"].push_back(item.toJson());
    }
    
    return response.dump();
}
```

### Return Single Item or 404
```cpp
std::string getById(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    auto item = service_->getById(id);
    return item ? item->toJson().dump() : (ctx.sendNotFound(), "");
}
```

### Validate UUID Format (manual)
```cpp
bool isValidUuid(const std::string& uuid) {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
        "[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    return std::regex_match(uuid, uuidRegex);
}
```

### Try-Catch in Handler
```cpp
std::string create(http::HttpContext& ctx) {
    try {
        auto body = parseJsonBody(ctx);
        if (!body) return "";
        
        auto item = service_->create(*body);
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
        return item.toJson().dump();
        
    } catch (const std::invalid_argument& e) {
        ctx.sendError(e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        return "";
    } catch (const std::exception& e) {
        ctx.sendError(e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        return "";
    }
}
```

## Testing

```cpp
#include <catch2/catch_all.hpp>

TEST_CASE("Route matches pattern") {
    http::Route route("GET", "/inventory/{id}", handler);
    REQUIRE(route.matches("GET", "/inventory/123"));
}

TEST_CASE("Extracts parameters") {
    http::Route route("GET", "/inventory/{id}", handler);
    auto params = route.extractParameters("/inventory/abc");
    REQUIRE(params["id"] == "abc");
}
```

## Documentation

- **README.md**: Getting started and overview
- **ARCHITECTURE.md**: Detailed architecture and design
- **MIGRATION_GUIDE.md**: Migrating existing services
- **SUMMARY.md**: Implementation summary
- **QUICK_REFERENCE.md**: This file

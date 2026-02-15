# HTTP Framework Architecture

## Overview

The HTTP Framework is a modern C++ library for building HTTP services, inspired by ASP.NET Core's architecture. It provides a clean separation of concerns through middleware, routing, and controllers.

## Design Philosophy

### 1. Separation of Concerns

- **Controllers**: Business logic and endpoint definitions
- **Middleware**: Cross-cutting concerns (auth, logging, CORS)
- **Router**: URL pattern matching and parameter extraction
- **HttpContext**: Request/response encapsulation

### 2. Developer Experience

- **Declarative routing**: Define routes in controller constructor
- **Automatic parameter extraction**: Route and query parameters available in context
- **Unified response handling**: Return JSON strings, framework handles HTTP details
- **Type safety**: C++20 with concepts and strong typing

### 3. Composability

- **Middleware pipeline**: Chain middleware in any order
- **Controller registration**: Add controllers independently
- **Extensibility**: Easy to add custom middleware and constraints

## Architecture Layers

```
┌─────────────────────────────────────────────────────────────┐
│                         HttpHost                             │
│  - Server lifecycle management                              │
│  - Component orchestration                                  │
└─────────────────┬───────────────────────────────────────────┘
                  │
    ┌─────────────┴─────────────┐
    │                           │
    ▼                           ▼
┌───────────────────┐   ┌──────────────────┐
│ MiddlewarePipeline│   │     Router        │
│  - Logging        │   │  - Pattern match  │
│  - Auth           │   │  - Param extract  │
│  - CORS           │   │  - Route table    │
│  - Error handling │   └──────────────────┘
└───────────────────┘            │
         │                       │
         │                       ▼
         │              ┌─────────────────┐
         │              │  ControllerBase │
         │              │  - Route defs   │
         │              │  - Handlers     │
         │              └─────────────────┘
         │                       │
         └───────┬───────────────┘
                 │
                 ▼
         ┌──────────────┐
         │  HttpContext  │
         │  - Request    │
         │  - Response   │
         │  - Parameters │
         └──────────────┘
```

## Component Details

### HttpHost

**Responsibility**: Server orchestration and lifecycle

**Key Features**:
- Manages Poco HTTPServer instance
- Maintains middleware pipeline and router
- Handles server configuration (threads, timeouts)
- Provides simple start/stop API

**Usage**:
```cpp
HttpHost host(8080);
host.use(middleware);
host.addController(controller);
host.start();  // Blocks
```

### MiddlewarePipeline

**Responsibility**: Execute middleware in order

**Key Features**:
- Ordered execution (FIFO)
- Async-style with continuations (next() callback)
- Short-circuit capability (don't call next())

**Execution Flow**:
```
Request → MW1 → MW2 → MW3 → Handler → MW3 → MW2 → MW1 → Response
```

**Built-in Middleware**:
1. **LoggingMiddleware**: Request/response logging with timing
2. **ErrorHandlingMiddleware**: Catch exceptions, return error JSON
3. **CorsMiddleware**: Add CORS headers
4. **AuthenticationMiddleware**: API key validation

### Router

**Responsibility**: Match URLs to handlers

**Key Features**:
- Pattern-based matching (e.g., `/inventory/{id}`)
- Route parameter extraction
- Constraint support (uuid, int, alpha, alphanum)
- Method differentiation (GET, POST, etc.)

**Route Pattern Syntax**:
```
/api/v1/inventory             # Literal
/api/v1/inventory/{id}        # Parameter
/api/v1/inventory/{id:uuid}   # Parameter with constraint
/api/v1/page/{page:int}       # Integer constraint
```

**Constraint Types**:
- `uuid`: Must match UUID format
- `int`: Must be numeric
- `alpha`: Letters only
- `alphanum`: Letters and numbers

### Route

**Responsibility**: Represent single route definition

**Key Features**:
- Regex-based matching (compiled at construction)
- Parameter extraction via named groups
- Constraint validation
- Handler function storage

**Internal Structure**:
```cpp
class Route {
    std::string method_;               // GET, POST, etc.
    std::string pattern_;              // /inventory/{id}
    EndpointHandler handler_;          // Lambda function
    std::vector<RouteParameter> params_;
    std::regex regex_;                 // Compiled pattern
};
```

### ControllerBase

**Responsibility**: Base class for grouping related endpoints

**Key Features**:
- Base route (e.g., `/api/v1/inventory`)
- Fluent endpoint registration (Get, Post, Put, Delete)
- Helper methods (parseJsonBody, validateRequiredFields)
- Automatic full path construction

**Pattern**:
```cpp
class MyController : public ControllerBase {
public:
    MyController() : ControllerBase("/api/v1/resource") {
        Get("/", [this](HttpContext& ctx) { ... });
        Get("/{id}", [this](HttpContext& ctx) { ... });
        Post("/", [this](HttpContext& ctx) { ... });
    }
};
```

### HttpContext

**Responsibility**: Encapsulate request/response with helpers

**Key Features**:
- References to Poco request/response
- Route parameters (extracted from URL)
- Query parameters (parsed from query string)
- Request-scoped data storage (items map)
- Helper methods for common operations

**Data Flow**:
```
Poco Request → HttpContext → Handler → JSON → HttpContext → Poco Response
```

**Helper Methods**:
- `getBodyAsString()`: Read request body
- `getBodyAsJson()`: Parse body as JSON
- `sendJson()`: Send JSON response
- `sendError()`: Send error response
- `sendNotFound()`: Send 404
- `sendCreated()`: Send 201 with Location header

## Request Lifecycle

### 1. Request Arrives

```
Client → Poco HTTPServer → FrameworkRequestHandler
```

### 2. Context Creation

```cpp
Poco::URI uri(request.getURI());
auto queryParams = uri.getQueryParameters();
HttpContext ctx(request, response, queryParams);
```

### 3. Middleware Pipeline

```cpp
middleware_.execute(ctx, [this, &ctx]() {
    processRequest(ctx);
});
```

Each middleware can:
- Inspect/modify request
- Short-circuit (don't call next)
- Execute code after handler (on return path)

### 4. Route Matching

```cpp
auto route = router_.findRoute(ctx.getMethod(), ctx.getPath());
if (!route) {
    send404(ctx.response, path);
    return;
}
```

### 5. Parameter Extraction

```cpp
ctx.routeParams = route->extractParameters(path);
```

### 6. Handler Execution

```cpp
std::string responseJson = route->getHandler()(ctx);
```

Handler can:
- Return JSON string
- Set status via `ctx.setStatus()`
- Send response directly via `ctx.sendJson()`

### 7. Response Sending

```cpp
if (!responseJson.empty()) {
    ctx.sendJson(responseJson);
}
```

### 8. Middleware Return Path

Pipeline unwinds, each middleware can add final touches.

## Performance Considerations

### 1. Route Compilation

Routes compiled to regex at registration time, not per-request:

```cpp
// Constructor - one time
Route::Route(...) {
    parsePattern();
    regex_ = std::regex(patternToRegex());
}

// Per-request - fast
bool Route::matches(...) {
    return std::regex_match(path, regex_);
}
```

### 2. Parameter Extraction

Parameters extracted via regex capture groups (efficient):

```cpp
std::smatch match;
std::regex_match(path, match, regex_);
for (size_t i = 0; i < parameters_.size(); ++i) {
    result[parameters_[i].name] = match[i + 1].str();
}
```

### 3. Body Reading

Request body cached after first read:

```cpp
std::string HttpContext::getBodyAsString() {
    if (bodyRead_) {
        return bodyCache_;
    }
    // Read and cache
}
```

### 4. Thread Safety

- Each request gets its own HttpContext (thread-safe)
- Router and middleware are read-only after start (thread-safe)
- Controllers should be stateless or use thread-safe services

## Extension Points

### Custom Middleware

```cpp
class CustomMiddleware : public Middleware {
public:
    void process(HttpContext& ctx, std::function<void()> next) override {
        // Before handler
        doSomething(ctx);
        
        next();
        
        // After handler
        doSomethingElse(ctx);
    }
};
```

### Custom Route Constraints

Currently hardcoded, but could be extended:

```cpp
enum class RouteConstraint {
    None, Uuid, Int, Alpha, AlphaNum,
    Custom  // Extension point
};
```

### Custom Response Helpers

Add to ControllerBase or HttpContext:

```cpp
class MyControllerBase : public http::ControllerBase {
protected:
    void sendPaginated(HttpContext& ctx, const json& items, 
                      int page, int total) {
        json response = {
            {"items", items},
            {"page", page},
            {"total", total}
        };
        ctx.sendJson(response);
    }
};
```

## Error Handling Strategy

### 1. Exception-Based

Handlers throw exceptions, middleware catches:

```cpp
// Handler
if (invalid) {
    throw std::invalid_argument("Invalid input");
}

// ErrorHandlingMiddleware catches all
```

### 2. Response-Based

Handler sends error response directly:

```cpp
if (invalid) {
    ctx.sendError("Invalid input", HTTP_BAD_REQUEST);
    return "";  // Empty response indicates already sent
}
```

### 3. Optional-Based

Use std::optional for not-found scenarios:

```cpp
std::optional<json> parseJsonBody(HttpContext& ctx);

auto bodyOpt = parseJsonBody(ctx);
if (!bodyOpt) {
    return "";  // Error already sent
}
```

## Testing Strategy

### Unit Testing Controllers

Mock HttpContext to test handlers:

```cpp
TEST_CASE("Controller handles valid request") {
    auto mockCtx = createMockContext("GET", "/inventory/123");
    mockCtx.routeParams["id"] = "123";
    
    auto controller = InventoryController(mockService);
    std::string result = controller.getById(mockCtx);
    
    REQUIRE(mockCtx.response.getStatus() == 200);
    json j = json::parse(result);
    REQUIRE(j["id"] == "123");
}
```

### Integration Testing

Use real HttpHost with test server:

```cpp
TEST_CASE("Full request lifecycle") {
    HttpHost host(8080);
    host.addController(testController);
    
    std::thread serverThread([&]() { host.start(); });
    
    // Make HTTP request
    auto response = httpClient.get("http://localhost:8080/test");
    
    REQUIRE(response.status == 200);
    
    host.stop();
    serverThread.join();
}
```

## Security Considerations

### 1. Input Validation

Always validate inputs in handlers:

```cpp
if (!validateRequiredFields(ctx, body, {"field1", "field2"})) {
    return "";  // Validation failed, error sent
}
```

### 2. Authentication

Use AuthenticationMiddleware for service-to-service:

```cpp
host.use(std::make_shared<AuthenticationMiddleware>(
    apiKey,
    {"/health", "/swagger.json"}  // Public endpoints
));
```

### 3. CORS

Configure CORS middleware appropriately:

```cpp
host.use(std::make_shared<CorsMiddleware>(
    "https://trusted-origin.com",  // Don't use * in production
    "GET, POST, PUT, DELETE",
    "Content-Type, Authorization"
));
```

### 4. Error Messages

Don't leak internal details:

```cpp
// Bad
catch (const std::exception& e) {
    ctx.sendError(e.what(), 500);  // Might expose internals
}

// Good
catch (const std::exception& e) {
    Logger::error("Internal error: {}", e.what());
    ctx.sendError("Internal server error", 500);
}
```

## Future Enhancements

### 1. Custom Route Constraints

Allow registration of custom constraint validators:

```cpp
router.addConstraint("email", [](const std::string& value) {
    return validateEmail(value);
});

Get("/{email:email}", handler);
```

### 2. Request/Response Interceptors

More granular than middleware:

```cpp
host.intercept("response", [](HttpContext& ctx) {
    ctx.setHeader("X-Service-Version", "1.0.0");
});
```

### 3. WebSocket Support

Extend to support WebSocket connections:

```cpp
WebSocket("/{id}", [](WebSocketContext& ctx) {
    // Handle WebSocket
});
```

### 4. OpenAPI Generation

Auto-generate OpenAPI spec from route definitions:

```cpp
Get("/{id:uuid}", handler)
    .description("Get item by ID")
    .response(200, "Success", ItemDto::schema())
    .response(404, "Not found");
```

### 5. Rate Limiting

Built-in rate limiting middleware:

```cpp
host.use(std::make_shared<RateLimitMiddleware>(
    100,  // requests per minute
    "ip"  // per IP address
));
```

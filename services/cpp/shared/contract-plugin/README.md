# Contract Plugin

**Version:** 1.0.0

A plugin for the http-framework that provides contract validation, claims serving, and OpenAPI/Swagger generation from contract definitions.

## Features

- **Contract Validation Middleware**: Validates HTTP responses against contract DTOs
- **Claims Endpoints**: Serves 5 endpoints for claims information (`/api/v1/claims/*`)
- **Swagger Endpoint**: Serves `/api/swagger.json` with auto-generated OpenAPI 3.0 spec from contracts
- **Configuration**: Environment variables or JSON configuration
- **Testable**: Mockable interfaces for unit testing

## Architecture

The plugin uses a layered architecture for separation of concerns:

```
HTTP Layer:
    ClaimsController (5 endpoints)
        ↓ delegates to
Business Logic:
    ClaimsService (IClaimsService)
        ↓ uses
Data Access:
    ClaimsLoader (IClaimsLoader)
        ↓ reads
Filesystem:
    claims.json
```

**Benefits:**
- **ClaimsLoader**: Mockable I/O layer for testing
- **ClaimsService**: Testable business logic without filesystem dependencies  
- **ClaimsController**: Thin HTTP layer that delegates to service

**Service Registration (via DI):**
```cpp
IClaimsLoader → ClaimsLoader (Singleton - shared across all services)
IClaimsService → ClaimsService (Transient - new instance per request)
```

### Claims Endpoints

The ClaimsController provides 5 endpoints matching inventory-service API:

1. `GET /api/v1/claims` - Full claims document
2. `GET /api/v1/claims/fulfilments` - Entity fulfilments section
3. `GET /api/v1/claims/references` - Entity references section
4. `GET /api/v1/claims/services` - Service contracts section
5. `GET /api/v1/claims/supports/{type}/{name}/{version}` - Support check

Example response for support check:
```json
{
  "type": "entity",
  "contract": "Product",
  "version": "1.0",
  "supported": true,
  "fulfilled": false
}
```

## Installation

### Build as Shared Library

```bash
cd services/cpp/shared/contract-plugin
mkdir build && cd build
cmake ..
make
```

This produces `libcontract-plugin.so` shared library.

### Include in Service

Option 1: Dynamic loading via PluginManager:

```cpp
#include <http-framework/PluginManager.hpp>

http::ServiceCollection services;
http::PluginManager pluginManager(services);

// Load plugin (registers services)
pluginManager.loadPlugin("./libcontract-plugin.so");

// Build provider
auto provider = services.buildServiceProvider();
```

Option 2: Direct linking (not recommended - defeats plugin architecture):

```cpp
#include <contract-plugin/ContractPlugin.hpp>

auto plugin = std::make_unique<contract::ContractPlugin>();
// Register manually...
```

## Usage Patterns

### Recommended: Per-Service Configuration (Direct Instantiation)

Each service configures the plugin with its own paths:

```cpp
#include "order/Application.hpp"
#include <contract-plugin/ContractPlugin.hpp>
#include <http-framework/ServiceScopeMiddleware.hpp>
#include <http-framework/Middleware.hpp>

void Application::initialize() {
    // =========================================================================
    // 1. Configure DI Container
    // =========================================================================
    http::ServiceCollection services;
    
    // Register your services...
    services.addService<utils::Database>(...);
    services.addScoped<services::IOrderService, services::OrderService>();
    
    // =========================================================================
    // 2. Create and Configure Contract Plugin
    // =========================================================================
    contractPlugin_ = std::make_shared<contract::ContractPlugin>();
    
    // Configure with SERVICE-SPECIFIC paths
    contract::ContractConfig contractConfig;
    contractConfig.claimsPath = "./claims.json";              // THIS service's claims
    contractConfig.contractsPath = "./contracts";             // THIS service's contracts
    contractConfig.globalContractsPath = "../../contracts";   // Shared global contracts
    contractConfig.enableValidation = true;
    contractConfig.strictMode = false;
    contractConfig.enableSwagger = true;
    contractConfig.swaggerTitle = "Order Service API";        // Service-specific title
    contractConfig.swaggerVersion = "1.0.0";
    contractConfig.swaggerDescription = "Order management endpoints";
    
    contractPlugin_->configure(contractConfig);
    
    // Register plugin services into DI container
    http::NamespacedServiceCollection pluginServices(services, "contract-plugin");
    contractPlugin_->registerServices(pluginServices);
    
    // =========================================================================
    // 3. Build Service Provider
    // =========================================================================
    serviceProvider_ = services.buildServiceProvider();
    
    // =========================================================================
    // 4. Configure HTTP Host
    // =========================================================================
    httpHost_ = std::make_unique<http::HttpHost>(serverPort_, serverHost_);
    
    httpHost_->use(std::make_shared<http::ServiceScopeMiddleware>(serviceProvider_));
    httpHost_->use(std::make_shared<http::ErrorHandlingMiddleware>());
    
    // Contract validation middleware (if enabled)
    if (contractConfig.enableValidation) {
        auto contractMiddleware = serviceProvider_->getService<contract::ContractValidationMiddleware>();
        httpHost_->use(contractMiddleware);
    }
    
    // =========================================================================
    // 5. Register Controllers (application + plugin)
    // =========================================================================
    httpHost_->addController(std::make_shared<controllers::OrderController>(*serviceProvider_));
    httpHost_->addController(std::make_shared<controllers::HealthController>(*serviceProvider_));
    
    // Add plugin controllers (claims, swagger)
    for (auto& controller : contractPlugin_->getControllers()) {
        httpHost_->addController(controller);
    }
}
```

**Key Points:**
- Each service instance specifies its own `claims.json` and `contracts/` directory
- Service-specific swagger title and description
- Plugin is a member variable of Application class
- No environment variables needed (pure programmatic config)

## Configuration

### Environment Variables

```bash
# Required paths
export CONTRACT_CLAIMS_PATH="./claims.json"
export CONTRACT_CONTRACTS_PATH="./contracts"
export CONTRACT_GLOBAL_CONTRACTS_PATH="../../contracts"

# Validation settings
export CONTRACT_ENABLE_VALIDATION="true"       # Enable response validation
export CONTRACT_STRICT_MODE="false"            # true = 500 on violation, false = log warning
export CONTRACT_LOG_VIOLATIONS="true"          # Log validation errors

# Endpoint settings
export CONTRACT_ENABLE_SWAGGER="true"          # Enable /api/swagger.json
export CONTRACT_ENABLE_CLAIMS="true"           # Enable /api/claims
```

### JSON Configuration

```json
{
  "claimsPath": "./claims.json",
  "contractsPath": "./contracts",
  "globalContractsPath": "../../contracts",
  "enableValidation": true,
  "strictMode": false,
  "logViolations": true,
  "enableSwagger": true,
  "swaggerTitle": "Order Service API",
  "swaggerVersion": "1.0.0",
  "swaggerDescription": "Order management endpoints",
  "enableClaims": true
}
```

## Usage in Application

### Complete Integration Example

```cpp
#include "order/Application.hpp"
#include <http-framework/PluginManager.hpp>
#include <http-framework/ServiceScopeMiddleware.hpp>
#include <http-framework/Middleware.hpp>

void Application::initialize() {
    // =========================================================================
    // 1. Configure DI Container
    // =========================================================================
    http::ServiceCollection services;
    
    // Register your services...
    services.addService<utils::Database>(...);
    services.addScoped<services::IOrderService, services::OrderService>();
    
    // =========================================================================
    // 2. Load Contract Plugin (BEFORE building provider)
    // =========================================================================
    http::PluginManager pluginManager(services);
    
    try {
        auto pluginInfo = pluginManager.loadPlugin("./libcontract-plugin.so");
        Logger::info("Loaded plugin: {} v{}", pluginInfo.name, pluginInfo.version);
    } catch (const std::exception& e) {
        Logger::warn("Contract plugin not available: {}", e.what());
    }
    
    // =========================================================================
    // 3. Build Service Provider (plugin services now available)
    // =========================================================================
    serviceProvider_ = services.buildServiceProvider();
    
    // =========================================================================
    // 4. Configure HTTP Host
    // =========================================================================
    httpHost_ = std::make_unique<http::HttpHost>(serverPort_, serverHost_);
    
    // ServiceScopeMiddleware FIRST
    httpHost_->use(std::make_shared<http::ServiceScopeMiddleware>(serviceProvider_));
    
    // ErrorHandlingMiddleware SECOND
    httpHost_->use(std::make_shared<http::ErrorHandlingMiddleware>());
    
    // Contract Validation Middleware (from plugin)
    try {
        auto contractMiddleware = serviceProvider_->getService<contract::ContractValidationMiddleware>();
        httpHost_->use(contractMiddleware);
        Logger::info("Contract validation middleware enabled");
    } catch (const std::exception& e) {
        Logger::debug("Contract validation middleware not available");
    }
    
    // =========================================================================
    // 5. Register Controllers (including plugin controllers)
    // =========================================================================
    httpHost_->addController(std::make_shared<controllers::OrderController>(*serviceProvider_));
    httpHost_->addController(std::make_shared<controllers::HealthController>(*serviceProvider_));
    
    // Plugin controllers (claims, swagger) are auto-registered by PluginManager
    // Or manually retrieve them:
    // auto pluginControllers = plugin->getControllers();
    // for (auto& controller : pluginControllers) {
    //     httpHost_->addController(controller);
    // }
}
```

### Minimal Integration (Without Plugin)

If you want to use the components without plugin architecture:

```cpp
#include <contract-plugin/ContractConfig.hpp>
#include <contract-plugin/ContractValidationMiddleware.hpp>
#include <contract-plugin/ClaimsController.hpp>
#include <contract-plugin/SwaggerController.hpp>

void Application::initialize() {
    // Configure
    contract::ContractConfig config;
    config.claimsPath = "./claims.json";
    config.contractsPath = "./contracts";
    config.enableValidation = true;
    config.strictMode = false;
    
    // Add middleware
    httpHost_->use(std::make_shared<contract::ContractValidationMiddleware>(config));
    
    // Add controllers
    httpHost_->addController(std::make_shared<contract::ClaimsController>(config));
    httpHost_->addController(std::make_shared<contract::SwaggerController>(config));
}
```

## Endpoints

### GET /api/claims

Returns the service's `claims.json` file with fulfilments and references.

**Response: 200 OK**

```json
{
  "service": "order-service",
  "version": "1.0.0",
  "fulfilments": [...],
  "references": [...]
}
```

### GET /api/swagger.json

Returns auto-generated OpenAPI 3.0 specification from contract files.

**Response: 200 OK**

```json
{
  "openapi": "3.0.3",
  "info": {
    "title": "Order Service API",
    "version": "1.0.0"
  },
  "paths": {
    "/api/v1/orders": {...},
    "/api/v1/orders/{id}": {...}
  },
  "components": {
    "schemas": {
      "OrderDto": {...},
      "OrderListDto": {...}
    }
  }
}
```

## Contract Validation

The middleware validates responses automatically:

1. **Intercepts responses** from controllers
2. **Loads endpoint contract** from `contracts/endpoints/`
3. **Validates response** against DTO definition
4. **Checks**:
   - Required fields present
   - Field types match (UUID, DateTime, etc.)
   - Referenced entity identity fields included
   - Enum values valid

### Validation Modes

**Lenient Mode** (`strictMode=false`, default):
- Logs violations as warnings
- Allows response through
- Good for development

**Strict Mode** (`strictMode=true`):
- Returns 500 error with validation details
- Blocks invalid responses
- Good for production with CI/CD validation

## File Structure

```
contract-plugin/
├── CMakeLists.txt
├── README.md
├── TESTING_GUIDE.md              # Guide for mocking ClaimsLoader in tests
├── INTEGRATION_EXAMPLES.md       # Per-service configuration examples
├── include/
│   └── contract-plugin/
│       ├── ContractConfig.hpp
│       ├── ContractValidationMiddleware.hpp
│       ├── IClaimsLoader.hpp              # Interface for loading claims.json (mockable)
│       ├── ClaimsLoader.hpp               # Filesystem implementation
│       ├── IClaimsService.hpp             # Interface for claims operations
│       ├── ClaimsService.hpp              # Business logic implementation
│       ├── ClaimsController.hpp           # HTTP endpoints (5 endpoints)
│       ├── SwaggerController.hpp
│       └── ContractPlugin.hpp
└── src/
    ├── ContractConfig.cpp
    ├── ContractValidationMiddleware.cpp
    ├── ClaimsLoader.cpp                   # Filesystem loading with directory search
    ├── ClaimsService.cpp                  # Claims operations and support checking
    ├── ClaimsController.cpp               # Delegates to ClaimsService
    ├── SwaggerController.cpp
    └── ContractPlugin.cpp
```

## Dependencies

- **http-framework**: Controller, middleware, plugin system
- **nlohmann/json**: JSON parsing
- **Poco**: HTTP server
- **spdlog**: Logging

## Development

### Adding New Validation Rules

Edit `ContractValidationMiddleware::validateFieldTypes()`:

```cpp
std::vector<std::string> ContractValidationMiddleware::validateFieldTypes(
    const json& responseJson,
    const json& contract) {
    
    std::vector<std::string> errors;
    
    // Add your validation logic:
    if (!responseJson.contains("requiredField")) {
        errors.push_back("Missing required field: requiredField");
    }
    
    return errors;
}
```

### Extending OpenAPI Generation

Edit `SwaggerController::endpointToPathItem()` to customize OpenAPI output.

## Testing

### Unit Testing with Mocks

The plugin is designed for testability with mockable interfaces:

- **IClaimsLoader**: Mock filesystem operations
- **IClaimsService**: Mock claims business logic

See [TESTING_GUIDE.md](./TESTING_GUIDE.md) for comprehensive examples of:
- Mocking ClaimsLoader to test ClaimsService without filesystem
- Testing entity support checking
- Testing filtered claims responses
- Integration testing with real filesystem

### Integration Testing

```bash
# Build
cd build && cmake .. && make

# Run service with plugin
cd /path/to/service
CONTRACT_CLAIMS_PATH="./claims.json" ./order-service

# Test endpoints
curl http://localhost:8082/api/claims
curl http://localhost:8082/api/swagger.json
```

## License

Same as warehouse-management project.

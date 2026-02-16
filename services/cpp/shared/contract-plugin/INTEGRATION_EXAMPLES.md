# Contract Plugin - Integration Examples

## Per-Service Configuration Examples

### Inventory Service Configuration

```cpp
// In Application.cpp or wherever you initialize the HTTP server

#include <contract-plugin/ContractPlugin.hpp>

void Application::initialize() {
    // ... DI setup ...
    
    // Create and configure plugin for INVENTORY SERVICE
    contractPlugin_ = std::make_shared<contract::ContractPlugin>();
    
    contract::ContractConfig config;
    config.claimsPath = "./claims.json";                              // inventory-service/claims.json
    config.contractsPath = "./contracts";                             // inventory-service/contracts/
    config.globalContractsPath = "../../contracts";                   // /contracts/ (shared)
    config.enableValidation = true;
    config.strictMode = false;                                        // Warn on validation errors
    config.logViolations = true;
    config.enableSwagger = true;
    config.swaggerTitle = "Inventory Service API";                    // Service-specific
    config.swaggerVersion = "1.0.0";
    config.swaggerDescription = "Inventory management and stock operations";
    config.enableClaims = true;
    
    contractPlugin_->configure(config);
    
    // Register plugin services
    http::NamespacedServiceCollection pluginServices(services, "contract-plugin");
    contractPlugin_->registerServices(pluginServices);
    
    // ... build provider ...
    
    // Add plugin controllers
    for (auto& controller : contractPlugin_->getControllers()) {
        httpHost_->addController(controller);
    }
}
```

**Directory Structure for Inventory:**
```
inventory-service/
├── claims.json                    ← config.claimsPath
├── contracts/
│   ├── dtos/
│   │   ├── InventoryItemDto.json
│   │   └── InventoryListDto.json
│   ├── requests/
│   │   └── ReserveInventoryRequest.json
│   ├── events/
│   │   └── InventoryReserved.json
│   └── endpoints/
│       ├── GetInventory.json
│       └── ListInventory.json
└── ../../contracts/               ← config.globalContractsPath (shared)
    ├── entities/v1/Inventory.json
    └── types/v1/UUID.json
```

### Order Service Configuration

```cpp
void Application::initialize() {
    contractPlugin_ = std::make_shared<contract::ContractPlugin>();
    
    contract::ContractConfig config;
    config.claimsPath = "./claims.json";                              // order-service/claims.json
    config.contractsPath = "./contracts";                             // order-service/contracts/
    config.globalContractsPath = "../../contracts";
    config.swaggerTitle = "Order Service API";                        // Different title
    config.swaggerDescription = "Order placement and fulfillment";
    
    contractPlugin_->configure(config);
    // ... rest of setup ...
}
```

### Product Service Configuration

```cpp
void Application::initialize() {
    contractPlugin_ = std::make_shared<contract::ContractPlugin>();
    
    contract::ContractConfig config;
    config.claimsPath = "./claims.json";                              // product-service/claims.json
    config.contractsPath = "./contracts";                             // product-service/contracts/
    config.globalContractsPath = "../../contracts";
    config.swaggerTitle = "Product Service API";                      // Different title
    config.swaggerDescription = "Product catalog and management";
    
    contractPlugin_->configure(config);
    // ... rest of setup ...
}
```

## Result: Service-Specific Endpoints

### Inventory Service - http://localhost:8081

- `GET /api/claims` → Returns inventory-service/claims.json
- `GET /api/swagger.json` → OpenAPI spec with title "Inventory Service API"
  - Paths from inventory-service/contracts/endpoints/
  - Schemas from inventory-service/contracts/dtos/

### Order Service - http://localhost:8082

- `GET /api/claims` → Returns order-service/claims.json
- `GET /api/swagger.json` → OpenAPI spec with title "Order Service API"
  - Paths from order-service/contracts/endpoints/
  - Schemas from order-service/contracts/dtos/

### Product Service - http://localhost:8083

- `GET /api/claims` → Returns product-service/claims.json
- `GET /api/swagger.json` → OpenAPI spec with title "Product Service API"
  - Paths from product-service/contracts/endpoints/
  - Schemas from product-service/contracts/dtos/

## Key Benefits

1. **Same Plugin, Different Config**: All services use `libcontract-plugin.so` but each has its own configuration
2. **No Collisions**: Each service's claims.json and contracts/ directory are independent
3. **Shared Contracts**: All services reference global contracts (entities, types) from `/contracts/`
4. **Service-Specific Swagger**: Each service has its own OpenAPI title and description

## Application Class Pattern

Store the plugin as a member variable:

```cpp
// Application.hpp
class Application {
public:
    void initialize();
    void run();
    void shutdown();

private:
    void initializeDI();
    void initializeHttpServer();
    
    std::shared_ptr<http::IServiceProvider> serviceProvider_;
    std::unique_ptr<http::HttpHost> httpHost_;
    std::shared_ptr<contract::ContractPlugin> contractPlugin_;  // Plugin instance
    
    std::string serverHost_;
    int serverPort_;
    std::string dbConnectionString_;
};
```

## Configuration Strategy

### Development (Lenient)
```cpp
config.enableValidation = true;
config.strictMode = false;   // Log warnings, allow responses
config.logViolations = true;
```

### Production (Strict)
```cpp
config.enableValidation = true;
config.strictMode = true;    // Return 500 on validation errors
config.logViolations = true;
```

### Disable Validation
```cpp
config.enableValidation = false;  // Skip validation entirely
```

## Testing the Plugin

```bash
# Start inventory service
cd inventory-service
./build/inventory-service

# Test claims endpoint (returns inventory-service/claims.json)
curl http://localhost:8081/api/claims | jq

# Test swagger endpoint
curl http://localhost:8081/api/swagger.json | jq '.info.title'
# Output: "Inventory Service API"

# Start order service
cd order-service  
./build/order-service

# Test claims endpoint (returns order-service/claims.json)
curl http://localhost:8082/api/claims | jq

# Test swagger endpoint
curl http://localhost:8082/api/swagger.json | jq '.info.title'
# Output: "Order Service API"
```

Each service serves its own contracts, but they all use the same plugin code!

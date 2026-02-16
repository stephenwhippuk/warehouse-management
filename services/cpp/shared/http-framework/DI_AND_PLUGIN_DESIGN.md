# Dependency Injection and Plugin System Design

## Design Goals

1. **Service Lifetimes**: Transient, Scoped (default), Singleton
2. **Service Provider Pattern**: Services receive `IServiceProvider&` to access dependencies
3. **Exception-Based**: Throw if service not found (bubbles to 500 error)
4. **Plugin System**: Dynamic loading with services and controllers
5. **Namespacing**: Plugin isolation and safety

---

## Part 1: Dependency Injection System

### Core API

```cpp
// Registration API
class ServiceCollection {
public:
    // Add service with explicit lifetime
    template<typename TInterface, typename TImplementation>
    void addService(ServiceLifetime lifetime = ServiceLifetime::Scoped);
    
    // Convenience methods
    template<typename TInterface, typename TImplementation>
    void addTransient();  // New instance every time
    
    template<typename TInterface, typename TImplementation>
    void addScoped();     // One instance per request scope (DEFAULT)
    
    template<typename TInterface, typename TImplementation>
    void addSingleton();  // One instance for application lifetime
    
    // Register with custom factory
    template<typename TInterface>
    void addService(std::function<std::shared_ptr<TInterface>(IServiceProvider&)> factory,
                    ServiceLifetime lifetime = ServiceLifetime::Scoped);
    
    // Build the provider
    std::shared_ptr<IServiceProvider> buildServiceProvider();
};

// Resolution API
class IServiceProvider {
public:
    virtual ~IServiceProvider() = default;
    
    // Get service (throws if not found)
    template<typename T>
    std::shared_ptr<T> getService();
    
    // Get service (returns nullptr if not found)
    template<typename T>
    std::shared_ptr<T> getOptionalService();
    
    // Create a new scope (for per-request services)
    virtual std::shared_ptr<IServiceScope> createScope() = 0;
};

// Scope management (per-request)
class IServiceScope {
public:
    virtual ~IServiceScope() = default;
    virtual IServiceProvider& getServiceProvider() = 0;
};

// Lifetime enum
enum class ServiceLifetime {
    Transient,  // New instance every call
    Scoped,     // One instance per scope (request)
    Singleton   // One instance for application
};
```

### Usage Example

```cpp
// In Server.cpp - Service registration
ServiceCollection services;

// Database connection pool (singleton)
services.addSingleton<IDatabase, PostgresDatabase>();

// Repositories (scoped - per request)
services.addScoped<IInventoryRepository, InventoryRepository>();
services.addScoped<IProductRepository, ProductRepository>();

// Services (scoped)
services.addScoped<IInventoryService, InventoryService>();

// Logger (transient - new instance each time)
services.addTransient<ILogger, SpdlogLogger>();

// Build provider
auto provider = services.buildServiceProvider();

// Use in HttpHost
httpHost->setServiceProvider(provider);
```

### Service Implementation Pattern

Services receive `IServiceProvider&` in constructor:

```cpp
class InventoryService : public IInventoryService {
public:
    explicit InventoryService(IServiceProvider& provider) 
        : provider_(provider) {
        // Resolve dependencies on demand
        repository_ = provider_.getService<IInventoryRepository>();
        logger_ = provider_.getService<ILogger>();
    }
    
    std::optional<dtos::InventoryItemDto> getById(const std::string& id) override {
        logger_->info("Fetching inventory: {}", id);
        auto inventory = repository_->findById(id);
        // ... business logic
    }

private:
    IServiceProvider& provider_;
    std::shared_ptr<IInventoryRepository> repository_;
    std::shared_ptr<ILogger> logger_;
};
```

### Controller Integration

Controllers receive `IServiceProvider&` but are NOT registered in DI container.
They're manually instantiated and added to HttpHost (keeps existing pattern):

```cpp
class InventoryController : public ControllerBase {
public:
    explicit InventoryController(IServiceProvider& provider)
        : provider_(provider) {}
    
    void handleGet(HttpContext& ctx) {
        requireServiceAuth(ctx);
        
        auto service = provider_.getService<IInventoryService>();
        auto result = service->getAll();
        
        // ... response handling
    }

private:
    IServiceProvider& provider_;
};

// In Server.cpp - manual instantiation (NOT via DI)
auto controller = std::make_shared<InventoryController>(*provider);
httpHost_->addController(controller);
```

### HttpContext Integration

Add scope to `HttpContext` for per-request services:

```cpp
class HttpContext {
public:
    // ... existing methods
    
    // Get scoped service provider for this request
    IServiceProvider& getServiceProvider() { return *scopedProvider_; }
    
    // Internal: Set scope for this request
    void setServiceScope(std::shared_ptr<IServiceScope> scope) {
        scope_ = scope;
        scopedProvider_ = &scope->getServiceProvider();
    }

private:
    std::shared_ptr<IServiceScope> scope_;
    IServiceProvider* scopedProvider_ = nullptr;
};
```

### Middleware Integration

Create scope per request:

```cpp
class ServiceScopeMiddleware : public Middleware {
public:
    explicit ServiceScopeMiddleware(std::shared_ptr<IServiceProvider> provider)
        : provider_(provider) {}
    
    void process(HttpContext& ctx, std::function<void()> next) override {
        // Create scope for this request
        auto scope = provider_->createScope();
        ctx.setServiceScope(scope);
        
        // Continue pipeline
        next();
        
        // Scope destroyed after request (RAII)
    }

private:
    std::shared_ptr<IServiceProvider> provider_;
};

// Usage in Server.cpp
httpHost->use(std::make_shared<ServiceScopeMiddleware>(provider));
```

---

## Part 2: Plugin System

### Plugin Architecture

```
Host Application
    ↓ loads .so/.dll
Plugin Library
    ↓ exports C function
registerServices(ServiceCollection&, PluginContext&)
    ↓ registers
Services + Controllers + Routes
```

### Plugin API

```cpp
// Plugin context - provides host capabilities to plugin
class PluginContext {
public:
    // Get plugin metadata
    std::string getName() const { return name_; }
    std::string getNamespace() const { return namespace_; }
    
    // Access to router for registering routes
    Router& getRouter() { return *router_; }
    
    // Access to middleware pipeline
    void addMiddleware(std::shared_ptr<Middleware> middleware);

private:
    std::string name_;
    std::string namespace_;
    Router* router_;
    HttpHost* host_;
};

// Plugin registration function signature
using PluginRegisterFunc = void(*)(ServiceCollection&, PluginContext&);

// Plugin info structure
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> dependencies;
};

// Plugin loader
class PluginLoader {
public:
    // Load single plugin
    void loadPlugin(const std::string& path, 
                   ServiceCollection& services,
                   HttpHost& host);
    
    // Load all plugins from directory
    void loadPluginsFromDirectory(const std::string& directory,
                                  ServiceCollection& services,
                                  HttpHost& host);
    
    // Get loaded plugin info
    std::vector<PluginInfo> getLoadedPlugins() const;

private:
    struct LoadedPlugin {
        void* handle;  // dlopen handle
        PluginInfo info;
        std::string path;
    };
    
    std::vector<LoadedPlugin> plugins_;
};
```

### Plugin Implementation Example

**inventory-plugin.cpp:**

```cpp
#include <http-framework/ServiceCollection.hpp>
#include <http-framework/PluginContext.hpp>
#include "InventoryService.hpp"
#include "InventoryController.hpp"

// Plugin metadata
extern "C" {
    http::PluginInfo getPluginInfo() {
        return {
            .name = "inventory-plugin",
            .version = "1.0.0",
            .description = "Inventory management plugin",
            .dependencies = {"database-plugin"}
        };
    }
    
    // Registration function
    void registerServices(http::ServiceCollection& services, 
                         http::PluginContext& context) {
        
        // Register services in plugin namespace
        auto ns = context.getNamespace();
        
        services.addScoped<IInventoryRepository, InventoryRepository>(ns);
        services.addScoped<IInventoryService, InventoryService>(ns);
        
        // Create and add controller (manual instantiation)
        auto& provider = context.getServiceProvider();
        auto controller = std::make_shared<InventoryController>(provider);
        context.addController(controller);
        
        // Add plugin-specific middleware
        context.addMiddleware(
            std::make_shared<InventoryAuthMiddleware>()
        );
    }
}
```

**CMakeLists.txt for plugin:**

```cmake
# Build as shared library
add_library(inventory-plugin SHARED
    inventory-plugin.cpp
    InventoryService.cpp
    InventoryController.cpp
    InventoryRepository.cpp
)

# Link against http-framework headers (not implementation)
target_link_libraries(inventory-plugin
    PRIVATE http-framework-interface
    PRIVATE pqxx
    PRIVATE nlohmann_json::nlohmann_json
)

# Install to plugins directory
install(TARGETS inventory-plugin
    LIBRARY DESTINATION plugins
)
```

### Host Application Integration

**Server.cpp:**

```cpp
#include <http-framework/ServiceCollection.hpp>
#include <http-framework/PluginLoader.hpp>
#include <http-framework/HttpHost.hpp>

int main() {
    // Create service collection
    http::ServiceCollection services;
    
    // Register core services
    services.addSingleton<IDatabase, PostgresDatabase>();
    services.addSingleton<ILogger, SpdlogLogger>();
    
    // Create HTTP host
    auto httpHost = std::make_shared<http::HttpHost>("0.0.0.0", 8080);
    
    // Load plugins BEFORE building provider
    http::PluginLoader pluginLoader;
    pluginLoader.loadPluginsFromDirectory("./plugins", services, *httpHost);
    
    // Build service provider (after all registrations)
    auto provider = services.buildServiceProvider();
    
    // Add service scope middleware
    httpHost->use(std::make_shared<http::ServiceScopeMiddleware>(provider));
    
    // Start server
    httpHost->start();
    
    return 0;
}
```

---

## Part 3: Namespacing for Safety

### Namespace Design

```cpp
class ServiceCollection {
public:
    // Register service in namespace
    template<typename TInterface, typename TImplementation>
    void addService(const std::string& ns, ServiceLifetime lifetime = ServiceLifetime::Scoped);
    
    // Create scoped collection for namespace
    ServiceNamespace createNamespace(const std::string& name);
};

class ServiceNamespace {
public:
    // All registrations go to parent's namespace
    template<typename TInterface, typename TImplementation>
    void addService(ServiceLifetime lifetime = ServiceLifetime::Scoped);
    
    std::string getName() const { return namespace_; }

private:
    std::string namespace_;
    ServiceCollection& parent_;
};

class IServiceProvider {
public:
    // Get service from default namespace
    template<typename T>
    std::shared_ptr<T> getService();
    
    // Get service from specific namespace
    template<typename T>
    std::shared_ptr<T> getService(const std::string& ns);
};
```

### Usage Example

```cpp
// Host registers in global namespace
services.addSingleton<IDatabase, PostgresDatabase>();  // global

// Plugin 1 registers in its namespace
auto plugin1 = services.createNamespace("inventory-plugin");
plugin1.addScoped<IInventoryService, InventoryService>();

// Plugin 2 registers in its namespace
auto plugin2 = services.createNamespace("order-plugin");
plugin2.addScoped<IOrderService, OrderService>();

// Resolution with namespace
auto inventoryService = provider->getService<IInventoryService>("inventory-plugin");
auto orderService = provider->getService<IOrderService>("order-plugin");

// Global services accessible from all namespaces
auto database = provider->getService<IDatabase>();  // Works from any namespace
```

### Plugin Isolation Rules

1. **Global Services**: Core services (Database, Logger) registered in global namespace, accessible to all plugins
2. **Plugin Services**: Each plugin's services in its own namespace, isolated from other plugins
3. **NO Cross-Plugin Access**: Plugins can ONLY access global/core services for security
   - Plugin A cannot access Plugin B's services
   - Enforced at runtime - throws exception if attempted

---

## Implementation Plan

### Phase 1: Core DI System (Foundation)
1. `ServiceLifetime.hpp` - Enum definition
2. `IServiceProvider.hpp` - Interface
3. `IServiceScope.hpp` - Scope interface
4. `ServiceDescriptor.hpp` - Internal descriptor class
5. `ServiceCollection.hpp` - Registration API
6. `ServiceProvider.cpp` - Implementation with `std::type_index` map

### Phase 2: Framework Integration
1. Update `HttpContext` - Add scope access
2. `ServiceScopeMiddleware` - Create scope per request
3. Update `ControllerBase` - Receive `IServiceProvider&`
4. Update `Router` - Pass provider to controllers

### Phase 3: Plugin System
1. `PluginContext.hpp` - Plugin capabilities
2. `PluginLoader.hpp` - Dynamic loading
3. `PluginLoader.cpp` - dlopen/LoadLibrary implementation

### Phase 4: Namespacing
1. Update `ServiceCollection` - Add namespace support
2. Update `ServiceProvider` - Namespace resolution
3. Add isolation rules and validation

### Phase 5: Testing & Examples
1. Unit tests for DI container
2. Integration tests with scopes
3. Example plugin implementation
4. Migration guide for inventory-service

---

## Open Questions

1. **Object Destruction Order**: Should we track dependency graph to ensure safe destruction?
   - Probably not needed - scopes naturally handle this with RAII

2. **Circular Dependencies**: How to detect/prevent?
   - Throw exception during registration if detected
   - Or allow with optional weak_ptr pattern?

3. **Thread Safety**: ✅ YES - Use `std::call_once` for singleton creation + mutex for container

4. **Service Replacement**: ✅ NO - Plugins cannot override core services (security)

5. **Plugin Unloading**: Should we support runtime plugin unload?
   - Complex - need to track all instances
   - Phase 2 feature, not MVP

6. **Validation**: Should we validate plugin dependencies before loading?
   - Yes - check required dependencies exist
   - Fail fast with clear error message

---

## Design Decisions Summary

1. ✅ **Service Provider Pattern**: Services receive `IServiceProvider&` and resolve dependencies on-demand
2. ✅ **Controllers**: Manual instantiation (NOT in DI), passed to `httpHost_->addController()`
3. ✅ **Namespacing**: Plugins isolated - can only access global/core services, not other plugins
4. ✅ **No Service Replacement**: Plugins cannot override core services (security)
5. ✅ **Thread Safety**: Use `std::call_once` for singleton creation + mutex protection
6. ✅ **Scoped by Default**: Per-request service lifetime is default
7. ✅ **Exception-Based**: Throw if service not found (bubbles to 500 error)

---

## Next Steps

Ready to implement **Phase 1: Core DI System** with thread-safe singleton creation.

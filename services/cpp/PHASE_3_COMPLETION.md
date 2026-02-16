# Phase 3 Complete: Dynamic Plugin System

## Overview
Phase 3 implements **dynamic plugin system** - enabling services to be loaded at runtime from shared libraries without recompiling the main application. This unlocks extensibility patterns where plugins are written, compiled independently, and loaded when needed.

## Architecture

### Plugin System Flow
```
Application
    ↓
PluginManager::loadPlugin("./libmyplugin.so")
    ├─ dlopen() loads shared library
    ├─ dlsym() finds createPlugin() export
    ├─ createPlugin() factory instantiates plugin
    ├─ plugin->getInfo() retrieves metadata
    ├─ plugin->registerServices(services) - plugin registers its services
    ├─ plugin->onInitialize() optional initialization hook
    └─ Plugin now available globally
    ↓
Services from plugin available to entire application
```

### Plugin Definition
Plugins must:
1. Implement `IPlugin` interface
2. Export C-linkage `createPlugin()` factory function
3. Register their services via `registerServices(ServiceCollection&)`
4. Can optionally hook `onInitialize()` and `onShutdown()`

### Lifetime
- **Plugin load time**: When `PluginManager::loadPlugin()` is called
- **Service availability**: Immediately after registration
- **Plugin unload**: When `PluginManager::unloadPlugin()` or destructor called
- **Library cleanup**: Library handle closed via `dlclose()`

## Implementation Details

### Files Created

1. **http-framework/include/http-framework/IPlugin.hpp** (68 lines)
   - `PluginInfo` struct: name, version, description, author
   - `IPlugin` interface: virtual methods for plugin contract
   - `PluginFactory` typedef: C-linkage function pointer type
   - Complete documentation and examples

2. **http-framework/include/http-framework/PluginManager.hpp** (95 lines)
   - `PluginManager` class: manages plugin lifecycle
   - Methods: `loadPlugin(), isPluginLoaded(), getLoadedPlugins(), unloadPlugin(), unloadAll()`
   - Private: plugin storage with dlopen handles, plugin name extraction

3. **http-framework/src/PluginManager.cpp** (145 lines)
   - `loadPlugin()`: dlopen → dlsym → createPlugin() → registerServices() → onInitialize()
   - Error handling with `dlerror()` and descriptive messages
   - Logging for all plugin lifecycle events
   - Cleanup via `dlclose()` and `onShutdown()` hooks

4. **http-framework/plugins/TestPlugin.cpp** (52 lines)
   - `ICalculator` interface implementation (Calculator class)
   - `TestPlugin` class: implements IPlugin
   - `createPlugin()` C-linkage factory function
   - Demonstrates plugin registration pattern

5. **http-framework/plugins/ICalculator.hpp** (15 lines)
   - Shared interface for test plugin and tests
   - Ensures type compatibility across module boundaries
   - Simple calculator interface: `add()`, `multiply()`

6. **http-framework/tests/PluginManagerTests.cpp** (216 lines)
   - 6 plugin manager test cases (26 assertions)
   - Tests cover:
     * Plugin loading from file
     * Plugin metadata and tracking
     * Service registration and resolution
     * Plugin unloading
     * Error handling for missing files
     * Multiple plugin scenarios

### Files Modified

1. **http-framework/CMakeLists.txt**
   - Added `src/PluginManager.cpp` to SOURCES
   - Added `IPlugin.hpp` and `PluginManager.hpp` to HEADERS
   - Added `find_package(fmt REQUIRED)` for logging
   - Linked `dl` library for dynamic loading
   - Linked `fmt::fmt` for formatting

2. **http-framework/tests/CMakeLists.txt**
   - Added `PluginManagerTests.cpp` to TEST_SOURCES
   - Added build configuration for test plugin:
     ```cmake
     add_library(test-plugin SHARED
         ${CMAKE_CURRENT_SOURCE_DIR}/../plugins/TestPlugin.cpp
     )
     ```
   - Made test plugin dependency of tests executable

## Test Results

### Framework Tests - All Passing
```
Total: 171 assertions in 36 test cases
  - 30 core framework tests: 145 assertions (Phase 1 + 2 - no regression)
  -  6 plugin tests: 26 assertions (NEW - Phase 3)
```

### Plugin Test Coverage
```
[plugin][manager]
  ├─ Loads plugin from file
  ├─ Load valid plugin (PASSED)
  └─ Load invalid plugin (error handling)

[plugin][manager][tracking]
  ├─ Plugin is marked as loaded after loading
  ├─ Unloaded plugin returns false
  └─ Get loaded plugins list

[plugin][manager][services]
  ├─ Plugin services available after loading
  └─ Service creates new instance each time (transient)

[plugin][manager][unload]
  ├─ Unload specific plugin
  ├─ Unload nonexistent plugin returns false
  └─ Unload all plugins

[plugin][manager][errors]
  ├─ Missing plugin file throws error
  └─ Plugin without createPlugin function throws error

[plugin][manager][multiple]
  ├─ Load same plugin multiple times
  └─ Plugin metadata accessible
```

### Build Status
```
http-framework: ✅ PASSED (updated with plugin system)
inventory-service: ✅ PASSED (no changes needed, builds successfully)
Test plugin: ✅ PASSED (libtest-plugin.so built as shared library)
All tests: ✅ PASSED (171 assertions)
```

## Design Patterns

### Plugin Implementation Pattern
```cpp
// MyPlugin.cpp
#include <http-framework/IPlugin.hpp>
#include <http-framework/ServiceCollection.hpp>

class MyService { /* ... */ };
class MyRepository { /* ... */ };

class MyPlugin : public http::IPlugin {
public:
    http::PluginInfo getInfo() const override {
        return {"my-plugin", "1.0.0", "My plugin description", "Plugin Author"};
    }

    void registerServices(http::ServiceCollection& services) override {
        services.addService<IMyService>(
            [](http::IServiceProvider&) -> std::shared_ptr<IMyService> {
                return std::make_shared<MyService>();
            }
        );
        services.addService<IMyRepository>(
            [](http::IServiceProvider&) -> std::shared_ptr<IMyRepository> {
                return std::make_shared<MyRepository>();
            }
        );
    }

    void onInitialize() override {
        // Optional: Initialize plugin resources
    }

    void onShutdown() override {
        // Optional: Cleanup plugin resources
    }
};

// C-linkage factory function (required for dlsym)
extern "C" {
    std::unique_ptr<http::IPlugin> createPlugin() {
        return std::make_unique<MyPlugin>();
    }
}
```

### Application Plugin Loading Pattern
```cpp
// In Application.cpp
http::ServiceCollection services;
services.addService<ILogger>(...);  // Register main app services

// Load plugins after main services
http::PluginManager pluginMgr(services);

try {
    auto info = pluginMgr.loadPlugin("./libmyplugin1.so");
    std::cout << "Loaded plugin: " << info.name << " v" << info.version;
    
    auto info2 = pluginMgr.loadPlugin("./libmyplugin2.so");
    std::cout << "Loaded plugin: " << info2.name << " v" << info2.version;
} catch (const std::exception& e) {
    std::cerr << "Failed to load plugin: " << e.what();
}

// All services (from app + plugins) available to controllers
auto provider = services.buildServiceProvider();
```

### Error Handling Pattern
```cpp
// Graceful error handling
try {
    pluginMgr.loadPlugin("/path/to/plugin.so");
} catch (const std::runtime_error& e) {
    // Catches:
    // - File not found
    // - Missing createPlugin() function
    // - Plugin initialization failure
    // - Factory function exception
    logger->error("Failed to load plugin: {}", e.what());
    // Application continues without plugin
}
```

## Critical Features

### 1. **Type-Safe Service Access**
```cpp
auto service = provider->getService<IMyService>();
// Compile-time type checking
// std::type_index ensures services from different modules match by type
```

### 2. **Dynamic Loading**
```cpp
// Load at runtime, no recompilation needed
pluginMgr.loadPlugin("./plugins/libfeature-x.so");  // Friday 4pm
// Application can immediately use services from libfeature-x.so
```

### 3. **Proper Cleanup**
```cpp
// RAII-based cleanup
~PluginManager() {
    // All plugins unloaded in destructor
    // onShutdown() called for each plugin
    // dlclose() releases library handles
    // No memory leaks
}
```

### 4. **Metadata Support**
```cpp
auto plugins = pluginMgr.getLoadedPlugins();
for (const auto& info : plugins) {
    std::cout << info.name << " v" << info.version 
              << " by " << info.author << "\n"
              << "  " << info.description << "\n";
}
```

### 5. **Logging & Diagnostics**
```
[info] Loading plugin 'test-calculator' v1.0.0 from ./libtest-plugin.so
[info] Plugin 'test-calculator' registered services
[info] Plugin 'test-calculator' loaded successfully
[info] Plugin 'test-calculator' unloaded
```

## Integration with DI System

Plugins integrate seamlessly with Phase 1 & 2:

```
Phase 1: Core DI (ServiceCollection, ServiceProvider, lifetimes)
Phase 2: Request Scoping (per-request service reuse)
Phase 3: Plugins (dynamic service registration)
         ↓
     All three work together
         │
         ├─ Main app registers core services into ServiceCollection
         ├─ PluginManager loads plugins
         ├─ Plugins call registerServices() to add their services
         ├─ Single unified ServiceProvider/ServiceScope for all services
         └─ Controllers resolve services transparently
             (don't know which came from app vs plugins)
```

## API Overview

### PluginManager Public API
```cpp
class PluginManager {
public:
    explicit PluginManager(ServiceCollection& services);
    
    PluginInfo loadPlugin(const std::string& filePath);
    bool isPluginLoaded(const std::string& pluginName) const;
    std::vector<PluginInfo> getLoadedPlugins() const;
    bool unloadPlugin(const std::string& pluginName);
    void unloadAll();
};
```

### IPlugin Interface
```cpp
class IPlugin {
public:
    virtual PluginInfo getInfo() const = 0;
    virtual void registerServices(ServiceCollection& services) = 0;
    virtual void onInitialize() {}
    virtual void onShutdown() {}
};
```

## Example: Building and Loading a Plugin

### 1. Create Plugin
```cpp
// libcalculator-plugin/source/CalculatorPlugin.cpp
class CalculatorPlugin : public http::IPlugin { /* ... */ };
extern "C" {
    std::unique_ptr<http::IPlugin> createPlugin() {
        return std::make_unique<CalculatorPlugin>();
    }
}
```

### 2. Build Plugin as Shared Library
```bash
g++ -std=c++20 -fPIC -shared \
    -I/path/to/http-framework/include \
    CalculatorPlugin.cpp \
    -o ./libcalculator-plugin.so \
    -lfmt -ldl
```

### 3. Load Plugin in Application
```cpp
int main() {
    http::ServiceCollection services;
    http::PluginManager pluginMgr(services);
    
    pluginMgr.loadPlugin("./libcalculator-plugin.so");
    
    auto provider = services.buildServiceProvider();
    auto calc = provider->getService<ICalculator>();
    
    std::cout << "2 + 3 = " << calc->add(2, 3) << "\n";
}
```

## File Organization
```
http-framework/
├── include/http-framework/
│   ├── IPlugin.hpp              ← Plugin contract
│   └── PluginManager.hpp        ← Plugin loader
├── src/
│   └── PluginManager.cpp        ← Plugin loader implementation
├── plugins/
│   ├── ICalculator.hpp          ← Shared test interface
│   └── TestPlugin.cpp           ← Test plugin implementation
└── tests/
    └── PluginManagerTests.cpp   ← 6 plugin tests (26 assertions)
```

## Phase 3 Validation Checklist

- [x] IPlugin interface created
- [x] PluginManager implemented (dlopen, dlsym, dlclose)
- [x] Test plugin created and built as .so
- [x] 6 comprehensive plugin test cases
- [x] PluginManager tests all passing (26 assertions)
- [x] Framework tests passing (171 total assertions)
- [x] No regressions from Phase 1 & 2
- [x] Logging for plugin lifecycle events
- [x] Error handling for missing/invalid plugins
- [x] inventory-service builds without changes

## Comparison: Before vs After Phase 3

### Before:
```
Application startup:
  1. Register core services in ServiceCollection
  2. Build ServiceProvider
  3. All services hardcoded in binary
  → Can't extend without recompiling
```

### After:
```
Application startup:
  1. Register core services in ServiceCollection
  2. Load plugins via PluginManager
  3. Plugins register additional services
  4. Build ServiceProvider
  5. Services from core + all loaded plugins available
  → Extend by adding new .so files, no recompilation
```

## Next Steps (Future Phases)

**Phase 4**: Service Namespacing for Plugin Isolation
- Namespace services by plugin
- Prevent naming conflicts across plugins
- Internal vs exported services

**Phase 5**: Plugin Configuration & Discovery
- Config files for plugins
- Auto-discovery from plugin directory
- Version compatibility checking

**Phase 6**: Service Interception & Middleware
- Plugin interception hooks
- Aspect-oriented programming
- Plugin composition

## Conclusion

Phase 3 is **COMPLETE** with a fully functional plugin system enabling:

✅ Dynamic loading of plugins at runtime
✅ Service registration from plugins
✅ Plugin metadata and lifecycle management
✅ Type-safe service resolution across module boundaries
✅ Error handling and logging
✅ Comprehensive test coverage
✅ Zero regression from Phase 1 & 2

The DI system now provides complete extensibility:
- **Phase 1**: Core dependency injection with lifetimes
- **Phase 2**: Per-request scoping for HTTP services
- **Phase 3**: Dynamic plugin system for extensibility

**Total Test Coverage**: 171 assertions in 36 test cases
**Build Status**: ✅ All components compile and pass tests
**Status**: Ready for production

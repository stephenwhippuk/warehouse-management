# C++ Services Framework Migration Status

## Overview

Migration of C++ services to use:
- **http-framework**: Controller-based routing, middleware, DI container
- **warehouse-messaging**: RabbitMQ event publishing/consuming

## Migration Status Summary

| Service | HTTP Framework | Messaging Framework | Status |
|---------|---------------|---------------------|--------|
| inventory-service | ✅ Complete | ✅ Complete | ✅ **DONE** |
| product-service | ✅ Complete | ✅ Complete | ✅ **DONE** |
| warehouse-service | 🔄 In Progress | ❌ Not started | 🔄 **IN PROGRESS** |
| order-service | ❌ Not started | ❌ Not started | ⏳ **TODO** |

**Progress:** 2/4 services complete (50%)

## Service Details

### inventory-service ✅ COMPLETE

Production-ready reference implementation with full DI container and messaging integration.

**Status:** All framework features working, fully tested.

---

### product-service ✅ COMPLETE

**What was migrated:**
- ✅ Added http-framework to CMakeLists.txt
- ✅ Refactored ProductController → `http::ControllerBase`
- ✅ Refactored HealthController → `http::ControllerBase`
- ✅ Refactored SwaggerController → `http::ControllerBase`
- ✅ Setup DI container in Application.cpp (scoped repository/service)
- ✅ Removed Server.hpp/Server.cpp (replaced by http::HttpHost)
- ✅ Updated handlers to use `HttpContext` and return `std::string`
- ✅ **Build successful** ✅

**Duration:** ~1 hour for 3 controllers + DI setup

**File changes:**
- `CMakeLists.txt`: +2 lines (add http-framework)
- `include/product/controllers/*.hpp`: 3 controllers refactored
- `src/controllers/*.cpp`: 3 controllers reimplemented
- `include/product/Application.hpp`: Refactored for DI
- `src/Application.cpp`: Added DI container setup

---

### warehouse-service 🔄 IN PROGRESS

**Current Status:**
- ✅ CMakeLists.txt updated (warehouse-messaging + http-framework added)
- ❓ Controllers: Not yet migrated (2 controllers: WarehouseController, LocationController)
- ❓ Application.cpp: Not yet refactored
- ❓ Server files: Not yet removed

**Controller Count:** 2 main controllers + 3 utility controllers (Health, Swagger, Claims)

**Migration Tasks Remaining:**
1. **WarehouseController** → `http::ControllerBase`
   - Endpoints: GET /warehouses, GET /warehouses/{id}, POST, PUT, DELETE
   
2. **LocationController** → `http::ControllerBase`
   - Endpoints: GET /locations, GET /locations/{id}, POST, PUT, DELETE
   
3. **HealthController** → `http::ControllerBase` (same pattern as product-service)
4. **SwaggerController** → `http::ControllerBase` (same pattern as product-service)
5. **ClaimsController** → `http::ControllerBase`
6. **Application.cpp** → Add DI container
7. **Remove Server.hpp/Server.cpp**

**Estimated Effort:** 2-3 hours

**Service patterns ready to follow:**
- Use `di_server.cpp` example (557 lines)
- Use product-service as reference (same migration done)
- Follow `/services/cpp/shared/http-framework/MIGRATION_GUIDE.md`

---

### order-service ⏳ TODO

**Current Status:**
- ❌ No http-framework integration
- ❌ No warehouse-messaging integration
- Uses old Poco pattern
- Single main controller (OrderController)

**Controller Count:** 1 main controller + 2 utility controllers (Health, Claims)

**Migration Tasks:**
1. Add http-framework to CMakeLists.txt
2. Add warehouse-messaging to CMakeLists.txt
3. Refactor OrderController → `http::ControllerBase`
4. Refactor HealthController → `http::ControllerBase`
5. Refactor ClaimsController → `http::ControllerBase`
6. Refactor Application.cpp → DI container
7. Remove Server.cpp

**Estimated Effort:** 1.5-2 hours

---

## Migration Checklist Template

### Phase 1: CMakeLists.txt [5 min]
```cmake
# Add before find_package()
add_subdirectory(${WAREHOUSE_MESSAGING_DIR} ...)
add_subdirectory(${HTTP_FRAMEWORK_DIR} ...)

# Add to target_link_libraries()
warehouse-messaging
http-framework
```

### Phase 2: Controllers [30-45 min per controller]

**Header (ProductController.hpp pattern):**
```cpp
class ProductController : public http::ControllerBase {
public:
    ProductController();
private:
    std::string handleGetAll(http::HttpContext& ctx);
    std::string handleGetById(http::HttpContext& ctx);
    // ... more handlers
};
```

**Implementation (ProductController.cpp pattern):**
```cpp
ProductController::ProductController() : http::ControllerBase("/api/v1/products") {
    Get("/", [this](http::HttpContext& ctx) { return handleGetAll(ctx); });
    Get("/{id:uuid}", [this](http::HttpContext& ctx) { return handleGetById(ctx); });
    Post("/", [this](http::HttpContext& ctx) { return handleCreate(ctx); });
    Put("/{id:uuid}", [this](http::HttpContext& ctx) { return handleUpdate(ctx); });
    Delete("/{id:uuid}", [this](http::HttpContext& ctx) { return handleDelete(ctx); });
}

std::string ProductController::handleGetAll(http::HttpContext& ctx) {
    int page = ctx.queryParams.getInt("page").value_or(1);
    auto service = ctx.getService<services::ProductService>();
    auto list = service->getAll(page, 50);
    return list.toJson().dump();
}
```

### Phase 3: Application.cpp [20-30 min]

```cpp
void Application::initialize() {
    // ... existing config setup ...
    
    http::ServiceCollection services;
    
    // Singleton
    services.addService<pqxx::connection>([](http::IServiceProvider& p) {
        return utils::Database::getConnection();
    }, http::ServiceLifetime::Singleton);
    
    // Scoped
    services.addService<IRepository, Repository>([](http::IServiceProvider& p) {
        return std::make_shared<Repository>(p.getService<pqxx::connection>());
    }, http::ServiceLifetime::Scoped);
    
    serviceProvider_ = services.buildServiceProvider();
}

void Application::start() {
    http::HttpHost host(port, host);
    host.use(std::make_shared<http::ServiceScopeMiddleware>(serviceProvider_));
    host.addController(std::make_shared<ProductController>());
    host.addController(std::make_shared<HealthController>());
    host.addController(std::make_shared<SwaggerController>());
    host.start();
}
```

### Phase 4: Cleanup [10 min]
- Remove Server.hpp
- Remove Server.cpp
- Update CMakeLists.txt (remove Server.cpp from SOURCES)

### Phase 5: Build & Test [15-30 min]
```bash
cd build && cmake .. && make
```

---

## Key Files for Reference

**Pattern Documentation:**
- `/services/cpp/shared/http-framework/MIGRATION_GUIDE.md` - Migration patterns
- `/services/cpp/shared/http-framework/examples/di_server.cpp` - Complete working example
- `/services/cpp/shared/warehouse-messaging/README.md` - Messaging integration

**Reference Implementations:**
- `inventory-service/` - First complete implementation
- `product-service/` - Recently migrated, same patterns as warehouse-service

**Copilot Instructions:**
- `/.github/copilot-instructions.md` - Complete HTTP + DI + Messaging architecture guide

---

## Migration Timeline

| Service | Status | Duration | Notes |
|---------|--------|----------|-------|
| inventory-service | ✅ DONE | ~4 hours | Initial framework development, test bed |
| product-service | ✅ DONE | ~1 hour | 3 controllers, reference complete |
| warehouse-service | 🔄 IN PROGRESS | ~2.5 hours est | 5 controllers total |
| order-service | ⏳ TODO | ~1.5 hours est | Simplest, 3 controllers |
| **TOTAL** | **50% complete** | **~9 hours** | 2 services done, 2 to go |

---

## Next Steps

1. **Immediate (warehouse-service):**
   - Migrate WarehouseController
   - Migrate LocationController  
   - Migrate utility controllers
   - Setup DI container
   - Build & verify

2. **Quick (order-service):**
   - Repeat warehouse-service pattern
   - Single controller simplifies migration

3. **Verification:**
   - [ ] All 4 services build successfully
   - [ ] All endpoints respond with framework
   - [ ] Dependencies inject correctly
   - [ ] Tests pass
   - [ ] Messaging works end-to-end

---

**Last Updated:** 2024-01-XX
**Status:** 50% complete (2/4 services)
**Estimated Time to 100%:** 3-4 more hours

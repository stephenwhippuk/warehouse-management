/**
 * @file di_server.cpp
 * @brief Complete example showing HTTP framework with Dependency Injection
 * 
 * Demonstrates:
 * - Service registration with different lifetimes (Singleton, Scoped, Transient)
 * - Service dependencies resolved automatically
 * - Per-request scoping with automatic cleanup
 * - Controller dependency injection
 * - Plugin loading with namespace isolation
 * 
 * Build:
 *   cd build
 *   cmake .. && make
 *   ./examples/di-server
 * 
 * Test:
 *   curl http://localhost:8080/api/health
 *   curl http://localhost:8080/api/v1/inventory
 *   curl http://localhost:8080/api/v1/inventory/550e8400-e29b-41d4-a716-446655440000
 */

#include "http-framework/HttpHost.hpp"
#include "http-framework/ControllerBase.hpp"
#include "http-framework/Middleware.hpp"
#include "http-framework/ServiceCollection.hpp"
#include "http-framework/PluginManager.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <vector>
#include <iostream>
#include <optional>
#include <chrono>

using json = nlohmann::json;

// ============================================================================
// Domain Models
// ============================================================================

struct InventoryItem {
    std::string id;
    std::string productId;
    std::string warehouseId;
    std::string locationId;
    int quantity;
    int reserved;
    
    json toJson() const {
        return {
            {"id", id},
            {"productId", productId},
            {"warehouseId", warehouseId},
            {"locationId", locationId},
            {"quantity", quantity},
            {"reserved", reserved},
            {"available", quantity - reserved}
        };
    }
};

// ============================================================================
// Service Interfaces (Abstract)
// ============================================================================

/**
 * @brief Database interface (Infrastructure Layer)
 * Lifetime: Singleton (shared connection pool)
 */
class IDatabase {
public:
    virtual ~IDatabase() = default;
    virtual bool isConnected() const = 0;
    virtual std::string getConnectionInfo() const = 0;
};

/**
 * @brief Logger interface (Infrastructure Layer)
 * Lifetime: Singleton (shared across app)
 */
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void info(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
};

/**
 * @brief Repository interface (Data Access Layer)
 * Lifetime: Scoped (per request, auto cleanup)
 */
class IInventoryRepository {
public:
    virtual ~IInventoryRepository() = default;
    virtual std::vector<InventoryItem> findAll() const = 0;
    virtual std::optional<InventoryItem> findById(const std::string& id) const = 0;
    virtual InventoryItem create(const InventoryItem& item) = 0;
    virtual bool reserve(const std::string& id, int quantity) = 0;
};

/**
 * @brief Business service interface (Business Logic Layer)
 * Lifetime: Scoped (per request)
 */
class IInventoryService {
public:
    virtual ~IInventoryService() = default;
    virtual std::vector<InventoryItem> getAll() const = 0;
    virtual std::optional<InventoryItem> getById(const std::string& id) const = 0;
    virtual InventoryItem create(const std::string& productId, int quantity) = 0;
    virtual bool reserve(const std::string& id, int quantity) = 0;
};

// ============================================================================
// Service Implementations
// ============================================================================

/**
 * @brief Mock database implementation
 * Demonstrates Singleton lifetime
 */
class MockDatabase : public IDatabase {
public:
    explicit MockDatabase(http::IServiceProvider&) {
        spdlog::info("[MockDatabase] ✅ Created (Singleton)");
    }
    
    ~MockDatabase() override {
        spdlog::info("[MockDatabase] 🗑️  Destroyed");
    }
    
    bool isConnected() const override {
        return true;
    }
    
    std::string getConnectionInfo() const override {
        return "Mock Database (In-Memory)";
    }
};

/**
 * @brief Simple logger implementation
 * Demonstrates Singleton lifetime
 */
class SimpleLogger : public ILogger {
public:
    explicit SimpleLogger(http::IServiceProvider&) {
        spdlog::info("[SimpleLogger] ✅ Created (Singleton)");
    }
    
    ~SimpleLogger() override {
        spdlog::info("[SimpleLogger] 🗑️  Destroyed");
    }
    
    void info(const std::string& message) override {
        spdlog::info("[App] {}", message);
    }
    
    void error(const std::string& message) override {
        spdlog::error("[App] {}", message);
    }
};

/**
 * @brief In-memory inventory repository
 * Demonstrates Scoped lifetime - new instance per request
 * Dependencies: IDatabase (injected), ILogger (injected)
 */
class InventoryRepository : public IInventoryRepository {
public:
    // Constructor receives IServiceProvider - dependencies auto-resolved
    explicit InventoryRepository(http::IServiceProvider& provider)
        : db_(provider.getService<IDatabase>())
        , logger_(provider.getService<ILogger>()) {
        
        spdlog::info("[InventoryRepository] ✅ Created (Scoped) - uses DB: {}", 
                     db_->getConnectionInfo());
        
        // Initialize mock data
        items_.push_back({"550e8400-e29b-41d4-a716-446655440000", "prod-001", "wh-001", "loc-001", 100, 0});
        items_.push_back({"550e8400-e29b-41d4-a716-446655440001", "prod-002", "wh-001", "loc-002", 50, 0});
        items_.push_back({"550e8400-e29b-41d4-a716-446655440002", "prod-003", "wh-002", "loc-001", 75, 10});
    }
    
    ~InventoryRepository() override {
        spdlog::info("[InventoryRepository] 🗑️  Destroyed (Scoped) - auto cleanup after request");
    }
    
    std::vector<InventoryItem> findAll() const override {
        logger_->info("Repository: Finding all inventory items");
        return items_;
    }
    
    std::optional<InventoryItem> findById(const std::string& id) const override {
        logger_->info("Repository: Finding inventory by ID: " + id);
        for (const auto& item : items_) {
            if (item.id == id) {
                return item;
            }
        }
        return std::nullopt;
    }
    
    InventoryItem create(const InventoryItem& item) override {
        logger_->info("Repository: Creating inventory item");
        items_.push_back(item);
        return item;
    }
    
    bool reserve(const std::string& id, int quantity) override {
        logger_->info("Repository: Reserving " + std::to_string(quantity) + " units");
        for (auto& item : items_) {
            if (item.id == id) {
                if (item.quantity - item.reserved >= quantity) {
                    item.reserved += quantity;
                    return true;
                }
            }
        }
        return false;
    }

private:
    std::shared_ptr<IDatabase> db_;  // Singleton - shared across requests
    std::shared_ptr<ILogger> logger_;  // Singleton - shared
    std::vector<InventoryItem> items_;  // Per-request state
};

/**
 * @brief Inventory business service
 * Demonstrates Scoped lifetime and service composition
 * Dependencies: IInventoryRepository (injected)
 */
class InventoryService : public IInventoryService {
public:
    explicit InventoryService(http::IServiceProvider& provider)
        : repository_(provider.getService<IInventoryRepository>()) {
        
        spdlog::info("[InventoryService] ✅ Created (Scoped)");
    }
    
    ~InventoryService() override {
        spdlog::info("[InventoryService] 🗑️  Destroyed (Scoped)");
    }
    
    std::vector<InventoryItem> getAll() const override {
        return repository_->findAll();
    }
    
    std::optional<InventoryItem> getById(const std::string& id) const override {
        return repository_->findById(id);
    }
    
    InventoryItem create(const std::string& productId, int quantity) override {
        InventoryItem item{
            generateId(),
            productId,
            "wh-001",
            "loc-001",
            quantity,
            0
        };
        return repository_->create(item);
    }
    
    bool reserve(const std::string& id, int quantity) override {
        // Business logic: validate quantity
        if (quantity <= 0) {
            return false;
        }
        
        auto item = repository_->findById(id);
        if (!item) {
            return false;
        }
        
        // Check availability
        if (item->quantity - item->reserved < quantity) {
            return false;
        }
        
        return repository_->reserve(id, quantity);
    }

private:
    std::shared_ptr<IInventoryRepository> repository_;
    static int nextId_;
    
    std::string generateId() const {
        return "550e8400-e29b-41d4-a716-44665544" + 
               std::to_string(1000 + nextId_++);
    }
};

int InventoryService::nextId_ = 3;

// ============================================================================
// Controllers
// ============================================================================

/**
 * @brief Health check controller
 * Shows simplest controller pattern
 */
class HealthController : public http::ControllerBase {
public:
    explicit HealthController(http::IServiceProvider& provider)
        : http::ControllerBase("/api")
        , provider_(provider) {
        
        Get("/health", [this](http::HttpContext& ctx) {
            return this->health(ctx);
        });
    }

private:
    http::IServiceProvider& provider_;
    
    std::string health(http::HttpContext& /* ctx */) {
        auto db = provider_.getService<IDatabase>();
        
        json response = {
            {"status", "healthy"},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"database", db->isConnected() ? "connected" : "disconnected"}
        };
        
        return response.dump();
    }
};

/**
 * @brief Inventory REST API controller
 * Demonstrates full CRUD with DI
 */
class InventoryController : public http::ControllerBase {
public:
    explicit InventoryController(http::IServiceProvider& provider)
        : http::ControllerBase("/api/v1/inventory")
        , provider_(provider) {
        
        spdlog::info("[InventoryController] ✅ Created");
        
        Get("/", [this](http::HttpContext& ctx) {
            return this->getAll(ctx);
        });
        
        Get("/{id:uuid}", [this](http::HttpContext& ctx) {
            return this->getById(ctx);
        });
        
        Post("/", [this](http::HttpContext& ctx) {
            return this->create(ctx);
        });
        
        Post("/{id:uuid}/reserve", [this](http::HttpContext& ctx) {
            return this->reserve(ctx);
        });
    }

private:
    http::IServiceProvider& provider_;
    
    std::string getAll(http::HttpContext& ctx) {
        spdlog::info("📨 Request: GET /api/v1/inventory - Listing all items");
        
        // Resolve service from REQUEST SCOPE
        auto service = ctx.getService<IInventoryService>();
        
        auto items = service->getAll();
        json j = json::array();
        for (const auto& item : items) {
            j.push_back(item.toJson());
        }
        
        spdlog::info("✅ Response: {} items returned", items.size());
        return j.dump();
    }
    
    std::string getById(http::HttpContext& ctx) {
        std::string id = ctx.routeParams["id"];
        spdlog::info("📨 Request: GET /api/v1/inventory/{}", id);
        
        auto service = ctx.getService<IInventoryService>();
        auto item = service->getById(id);
        
        if (!item) {
            spdlog::warn("⚠️  Item not found: {}", id);
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            return R"({"error": "Item not found"})";        }
        
        spdlog::info("✅ Response: Item found");
        return item->toJson().dump();
    }
    
    std::string create(http::HttpContext& ctx) {
        spdlog::info("📨 Request: POST /api/v1/inventory - Creating item");
        
        json body = ctx.getBodyAsJson();
        std::string productId = body["productId"];
        int quantity = body["quantity"];
        
        auto service = ctx.getService<IInventoryService>();
        auto item = service->create(productId, quantity);
        
        spdlog::info("✅ Response: Item created with ID {}", item.id);
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
        return item.toJson().dump();
    }
    
    std::string reserve(http::HttpContext& ctx) {
        std::string id = ctx.routeParams["id"];
        json body = ctx.getBodyAsJson();
        int quantity = body["quantity"];
        
        spdlog::info("📨 Request: POST /api/v1/inventory/{}/reserve (quantity: {})", id, quantity);
        
        auto service = ctx.getService<IInventoryService>();
        bool success = service->reserve(id, quantity);
        
        if (!success) {
            spdlog::warn("⚠️  Reservation failed");
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);
            return R"({"error": "Insufficient quantity"})";        }
        
        spdlog::info("✅ Response: Reserved {} units", quantity);
        json response = {{"success", true}, {"reserved", quantity}};
        return response.dump();
    }
};

// ============================================================================
// Application
// ============================================================================

int main() {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("🚀 Starting DI-Enabled HTTP Server Example");
    spdlog::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    try {
        // ====================================================================
        // Step 1: Configure Services (Dependency Injection Container)
        // ====================================================================
        
        spdlog::info("📦 [Step 1] Configuring services...");
        http::ServiceCollection services;
        
        // Infrastructure Layer (Singleton - shared across app)
        spdlog::info("   → Registering IDatabase (Singleton)");
        services.addService<IDatabase, MockDatabase>(
            http::ServiceLifetime::Singleton
        );
        
        spdlog::info("   → Registering ILogger (Singleton)");
        services.addService<ILogger, SimpleLogger>(
            http::ServiceLifetime::Singleton
        );
        
        // Data Access Layer (Scoped - per request)
        spdlog::info("   → Registering IInventoryRepository (Scoped)");
        services.addService<IInventoryRepository, InventoryRepository>(
            http::ServiceLifetime::Scoped
        );
        
        // Business Logic Layer (Scoped - per request)
        spdlog::info("   → Registering IInventoryService (Scoped)");
        services.addService<IInventoryService, InventoryService>(
            http::ServiceLifetime::Scoped
        );
        
        // ====================================================================
        // Step 2: Build Service Provider
        // ====================================================================
        
        spdlog::info("🏗️  [Step 2] Building service provider...");
        auto provider = services.buildServiceProvider();
        
        // Eagerly create singletons (connection pools, etc.)
        spdlog::info("   → Creating singleton instances:");
        auto db = provider->getService<IDatabase>();
        auto logger = provider->getService<ILogger>();
        
        logger->info("Service provider ready");
        spdlog::info("   ✅ Database: {}", db->getConnectionInfo());
        
        // ====================================================================
        // Step 3: Configure HTTP Host
        // ====================================================================
        
        spdlog::info("🌐 [Step 3] Configuring HTTP host...");
        http::HttpHost host(8088, provider, "0.0.0.0");
        spdlog::info("   → Default middleware enabled (ServiceScope + ErrorHandling)");
        
        // ====================================================================
        // Step 4: Register Controllers (resolved from DI)
        // ====================================================================
        
        spdlog::info("🎯 [Step 4] Registering controllers...");
        
        auto healthController = std::make_shared<HealthController>(*provider);
        host.addController(healthController);
        spdlog::info("   ✅ HealthController registered at /api/health");
        
        auto inventoryController = std::make_shared<InventoryController>(*provider);
        host.addController(inventoryController);
        spdlog::info("   ✅ InventoryController registered at /api/v1/inventory");
        
        // ====================================================================
        // Step 5: Optional - Load Plugins
        // ====================================================================
        
        spdlog::info("🔌 [Step 5] Plugin system ready");
        // Note: PluginManager would typically be used before building the provider
        // to allow plugins to register services into the ServiceCollection.
        // Example:
        // http::PluginManager pluginManager(services);
        // auto pluginInfo = pluginManager.loadPlugin("/path/to/plugin.so");
        // spdlog::info("   ✅ Plugin loaded: {}", pluginInfo.name);
        
        // ====================================================================
        // Step 6: Start Server
        // ====================================================================
        
        spdlog::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        spdlog::info("🎉 Server running at http://localhost:8088");
        spdlog::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        spdlog::info("");
        spdlog::info("Try these endpoints:");
        spdlog::info("  curl http://localhost:8088/api/health");
        spdlog::info("  curl http://localhost:8088/api/v1/inventory");
        spdlog::info("  curl http://localhost:8088/api/v1/inventory/550e8400-e29b-41d4-a716-446655440000");
        spdlog::info("");
        spdlog::info("Watch the logs to see:");
        spdlog::info("  ✅ Singleton services created once");
        spdlog::info("  ✅ Scoped services created per request");
        spdlog::info("  🗑️  Scoped services destroyed after response");
        spdlog::info("");
        spdlog::info("Press Ctrl+C to stop");
        spdlog::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        host.start();
        
    } catch (const std::exception& e) {
        spdlog::error("❌ Fatal error: {}", e.what());
        return 1;
    }
    
    return 0;
}

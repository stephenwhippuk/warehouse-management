#include "http-framework/HttpHost.hpp"
#include "http-framework/ControllerBase.hpp"
#include "http-framework/Middleware.hpp"
#include "http-framework/ServiceCollection.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <vector>
#include <iostream>

using json = nlohmann::json;

// ============================================================================
// Mock Service Layer (for demonstration)
// ============================================================================

struct Item {
    std::string id;
    std::string name;
    int quantity;
    
    json toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"quantity", quantity}
        };
    }
};

class InventoryService {
public:
    InventoryService() {
        // Initialize with some mock data
        items_.push_back({"550e8400-e29b-41d4-a716-446655440000", "Widget A", 100});
        items_.push_back({"550e8400-e29b-41d4-a716-446655440001", "Widget B", 50});
        items_.push_back({"550e8400-e29b-41d4-a716-446655440002", "Widget C", 25});
    }
    
    std::vector<Item> getAll() const {
        return items_;
    }
    
    std::optional<Item> getById(const std::string& id) const {
        for (const auto& item : items_) {
            if (item.id == id) {
                return item;
            }
        }
        return std::nullopt;
    }
    
    Item create(const std::string& name, int quantity) {
        Item item{generateId(), name, quantity};
        items_.push_back(item);
        return item;
    }
    
    bool update(const std::string& id, const std::string& name, int quantity) {
        for (auto& item : items_) {
            if (item.id == id) {
                item.name = name;
                item.quantity = quantity;
                return true;
            }
        }
        return false;
    }
    
    bool deleteById(const std::string& id) {
        auto it = std::remove_if(items_.begin(), items_.end(),
            [&id](const Item& item) { return item.id == id; });
        if (it != items_.end()) {
            items_.erase(it, items_.end());
            return true;
        }
        return false;
    }
    
    bool reserve(const std::string& id, int quantity) {
        for (auto& item : items_) {
            if (item.id == id) {
                if (item.quantity >= quantity) {
                    item.quantity -= quantity;
                    return true;
                }
            }
        }
        return false;
    }

private:
    std::vector<Item> items_;
    int nextId_ = 3;
    
    std::string generateId() {
        return "550e8400-e29b-41d4-a716-44665544000" + std::to_string(nextId_++);
    }
};

// ============================================================================
// Controllers
// ============================================================================

/**
 * @brief Inventory Controller - manages inventory items
 * 
 * Routes:
 * GET    /api/v1/inventory             - Get all items
 * GET    /api/v1/inventory/{id}        - Get item by ID
 * POST   /api/v1/inventory             - Create new item
 * PUT    /api/v1/inventory/{id}        - Update item
 * DELETE /api/v1/inventory/{id}        - Delete item
 * POST   /api/v1/inventory/{id}/reserve - Reserve inventory
 */
class InventoryController : public http::ControllerBase {
public:
    explicit InventoryController(std::shared_ptr<InventoryService> service)
        : http::ControllerBase("/api/v1/inventory"), service_(service) {
        
        // Register endpoints
        Get("/", [this](http::HttpContext& ctx) {
            return this->getAll(ctx);
        });
        
        Get("/{id:uuid}", [this](http::HttpContext& ctx) {
            return this->getById(ctx);
        });
        
        Post("/", [this](http::HttpContext& ctx) {
            return this->create(ctx);
        });
        
        Put("/{id:uuid}", [this](http::HttpContext& ctx) {
            return this->update(ctx);
        });
        
        Delete("/{id:uuid}", [this](http::HttpContext& ctx) {
            return this->deleteById(ctx);
        });
        
        Post("/{id:uuid}/reserve", [this](http::HttpContext& ctx) {
            return this->reserve(ctx);
        });
    }

private:
    std::shared_ptr<InventoryService> service_;
    
    std::string getAll(http::HttpContext& /* ctx */) {
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
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            return R"({"error": "Item not found"})";
        }
        
        return item->toJson().dump();
    }
    
    std::string create(http::HttpContext& ctx) {
        auto bodyOpt = parseJsonBody(ctx);
        if (!bodyOpt) return "";  // Error already sent
        
        json body = *bodyOpt;
        
        if (!validateRequiredFields(ctx, body, {"name", "quantity"})) {
            return "";  // Error already sent
        }
        
        std::string name = body["name"];
        int quantity = body["quantity"];
        
        auto item = service_->create(name, quantity);
        
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
        ctx.setHeader("Location", "/api/v1/inventory/" + item.id);
        
        return item.toJson().dump();
    }
    
    std::string update(http::HttpContext& ctx) {
        std::string id = ctx.routeParams["id"];
        
        auto bodyOpt = parseJsonBody(ctx);
        if (!bodyOpt) return "";
        
        json body = *bodyOpt;
        
        if (!validateRequiredFields(ctx, body, {"name", "quantity"})) {
            return "";
        }
        
        std::string name = body["name"];
        int quantity = body["quantity"];
        
        if (!service_->update(id, name, quantity)) {
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            return R"({"error": "Item not found"})";
        }
        
        return R"({"success": true, "message": "Item updated"})";
    }
    
    std::string deleteById(http::HttpContext& ctx) {
        std::string id = ctx.routeParams["id"];
        
        if (!service_->deleteById(id)) {
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
            return R"({"error": "Item not found"})";
        }
        
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
        return "";
    }
    
    std::string reserve(http::HttpContext& ctx) {
        std::string id = ctx.routeParams["id"];
        
        auto bodyOpt = parseJsonBody(ctx);
        if (!bodyOpt) return "";
        
        json body = *bodyOpt;
        
        if (!validateRequiredFields(ctx, body, {"quantity"})) {
            return "";
        }
        
        int quantity = body["quantity"];
        
        if (!service_->reserve(id, quantity)) {
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);
            return R"({"error": "Insufficient quantity or item not found"})";
        }
        
        return R"({"success": true, "message": "Inventory reserved"})";
    }
};

/**
 * @brief Health Controller - simple health check endpoint
 */
class HealthController : public http::ControllerBase {
public:
    HealthController() : http::ControllerBase("/health") {
        Get("/", [this](http::HttpContext& ctx) {
            return this->healthCheck(ctx);
        });
    }

private:
    std::string healthCheck(http::HttpContext& /* ctx */) {
        json response = {
            {"status", "healthy"},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        };
        return response.dump();
    }
};

// ============================================================================
// Main Application
// ============================================================================

int main() {
    try {
        // Create HTTP host on port 8085 (8080 might be in use)
        http::ServiceCollection services;
        auto provider = services.buildServiceProvider();
        http::HttpHost host(8085, provider);
        
        // Add middleware (ServiceScope + ErrorHandling are added by HttpHost)
        host.use(std::make_shared<http::LoggingMiddleware>());
        host.use(std::make_shared<http::CorsMiddleware>());
        
        // Optionally add authentication (uncomment to enable)
        // host.use(std::make_shared<http::AuthenticationMiddleware>("your-api-key-here"));
        
        // Create services
        auto inventoryService = std::make_shared<InventoryService>();
        
        // Add controllers
        host.addController(std::make_shared<InventoryController>(inventoryService));
        host.addController(std::make_shared<HealthController>());
        
        // Configure server
        host.setMaxThreads(16);
        host.setMaxQueued(100);
        host.setTimeout(60);
        
        // Start server (blocks until Ctrl+C)
        host.start();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

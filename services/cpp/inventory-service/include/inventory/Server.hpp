#pragma once

#include "inventory/services/InventoryService.hpp"
#include <Poco/Net/HTTPServer.h>
#include <memory>
#include <string>

namespace inventory {

enum class RouteTarget {
    Inventory,
    Health,
    Swagger
};

// Pure helper used by the HTTP server routing logic; exposed so
// tests can verify that individual paths map to the expected targets.
inline RouteTarget resolveRoute(const std::string& path) {
    if (path == "/health") {
        return RouteTarget::Health;
    }
    if (path == "/api/swagger.json") {
        return RouteTarget::Swagger;
    }
    return RouteTarget::Inventory;
}

class Server {
public:
    explicit Server(int port);
    ~Server();
    
    void setInventoryService(std::shared_ptr<services::InventoryService> service);
    void start();
    void stop();
    
private:
    int port_;
    std::unique_ptr<Poco::Net::HTTPServer> httpServer_;
    std::shared_ptr<services::InventoryService> inventoryService_;
};

} // namespace inventory

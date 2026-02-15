#pragma once

#include "inventory/services/InventoryService.hpp"
#include <http-framework/HttpHost.hpp>
#include <memory>
#include <string>

namespace inventory {

class Server {
public:
    explicit Server(int port);
    ~Server();
    
    void setInventoryService(std::shared_ptr<services::InventoryService> service);
    void start();
    void stop();
    
private:
    int port_;
    std::unique_ptr<http::HttpHost> httpHost_;
    std::shared_ptr<services::InventoryService> inventoryService_;
};

} // namespace inventory

#pragma once

#include "inventory/services/InventoryService.hpp"
#include <Poco/Net/HTTPServer.h>
#include <memory>

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
    std::unique_ptr<Poco::Net::HTTPServer> httpServer_;
    std::shared_ptr<services::InventoryService> inventoryService_;
};

} // namespace inventory

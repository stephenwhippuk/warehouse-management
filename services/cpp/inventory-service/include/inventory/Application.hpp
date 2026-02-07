#pragma once

#include "inventory/controllers/InventoryController.hpp"
#include "inventory/services/InventoryService.hpp"
#include "inventory/repositories/InventoryRepository.hpp"
#include "inventory/utils/MessageBus.hpp"
#include <memory>
#include <string>

namespace inventory {

class Application {
public:
    Application();
    ~Application();
    
    void initialize(const std::string& configPath);
    void run();
    void shutdown();
    
private:
    void loadConfiguration(const std::string& configPath);
    void initializeLogging();
    void initializeDatabase();
    void initializeServices();
    
    // Services
    std::shared_ptr<repositories::InventoryRepository> inventoryRepository_;
    std::shared_ptr<services::InventoryService> inventoryService_;
    std::shared_ptr<utils::MessageBus> messageBus_;
    
    // Configuration
    std::string dbConnectionString_;
    int serverPort_;
    std::string logLevel_;
    utils::MessageBus::Config messageBusConfig_;
    
    bool initialized_;
};

} // namespace inventory

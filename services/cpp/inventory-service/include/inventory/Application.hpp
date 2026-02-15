#pragma once

#include "inventory/controllers/InventoryController.hpp"
#include "inventory/services/InventoryService.hpp"
#include "inventory/repositories/InventoryRepository.hpp"
#include "inventory/handlers/ProductEventHandler.hpp"
#include <warehouse/messaging/EventPublisher.hpp>
#include <warehouse/messaging/EventConsumer.hpp>
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
    std::shared_ptr<warehouse::messaging::EventPublisher> eventPublisher_;
    std::unique_ptr<warehouse::messaging::EventConsumer> eventConsumer_;
    std::shared_ptr<handlers::ProductEventHandler> productEventHandler_;
    
    // Configuration
    std::string dbConnectionString_;
    int serverPort_;
    std::string logLevel_;
    
    bool initialized_;
};

} // namespace inventory

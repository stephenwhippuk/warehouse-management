#pragma once

#include "inventory/handlers/ProductEventHandler.hpp"
#include <warehouse/messaging/EventPublisher.hpp>
#include <warehouse/messaging/EventConsumer.hpp>
#include <http-framework/IServiceProvider.hpp>
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
    
    // Dependency injection
    std::shared_ptr<http::IServiceProvider> serviceProvider_;
    
    // Legacy services (for event consumer setup)
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

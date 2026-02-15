#include "inventory/Application.hpp"
#include "inventory/utils/Config.hpp"
#include "inventory/utils/Logger.hpp"
#include "inventory/utils/Database.hpp"
#include "inventory/Server.hpp"
#include <warehouse/messaging/EventConsumer.hpp>
#include <warehouse/messaging/EventPublisher.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

namespace inventory {

Application::Application() : initialized_(false), serverPort_(8080) {}

Application::~Application() {
    shutdown();
}

void Application::initialize(const std::string& configPath) {
    if (initialized_) {
        return;
    }
    
    loadConfiguration(configPath);
    initializeLogging();
    initializeDatabase();
    initializeServices();
    
    initialized_ = true;
    utils::Logger::info("Inventory Service initialized successfully");
}

void Application::run() {
    if (!initialized_) {
        throw std::runtime_error("Application not initialized");
    }
    
    utils::Logger::info("Starting Inventory Service on port {}", serverPort_);
    
    Server server(serverPort_);
    server.setInventoryService(inventoryService_);
    server.start();
    
    utils::Logger::info("Inventory Service stopped");
}

void Application::shutdown() {
    if (!initialized_) {
        return;
    }
    
    utils::Logger::info("Shutting down Inventory Service");
    
    // Stop event consumer gracefully
    if (eventConsumer_ && eventConsumer_->isRunning()) {
        eventConsumer_->stop();
    }
    
    utils::Database::disconnect();
    initialized_ = false;
}

void Application::loadConfiguration(const std::string& configPath) {
    utils::Config::load(configPath);
    
    // Load database configuration
    dbConnectionString_ = utils::Config::getEnv("DATABASE_URL",
        utils::Config::getString("database.connectionString", 
            "postgresql://inventory:password@localhost:5432/inventory_db"));
    
    // Load server configuration
    serverPort_ = utils::Config::getInt("server.port", 8080);
    
    // Load logging configuration
    logLevel_ = utils::Config::getString("logging.level", "info");

    // Note: Message bus configuration now handled by warehouse-messaging library via environment variables
    // The library reads RABBITMQ_HOST, RABBITMQ_PORT, RABBITMQ_USER, RABBITMQ_PASSWORD automatically
}

void Application::initializeLogging() {
    utils::Logger::init(logLevel_);
}

void Application::initializeDatabase() {
    utils::Logger::info("Connecting to database...");
    auto db = utils::Database::connect(dbConnectionString_);
    utils::Logger::info("Database connected successfully");
}

void Application::initializeServices() {
    auto db = utils::Database::getConnection();
    
    // Initialize repositories
    inventoryRepository_ = std::make_shared<repositories::InventoryRepository>(db);

    // Initialize event publisher (for publishing inventory events)
    utils::Logger::info("Initializing event publisher...");
    eventPublisher_ = std::shared_ptr<warehouse::messaging::EventPublisher>(
        warehouse::messaging::EventPublisher::create("inventory-service")
    );

    // Initialize services
    inventoryService_ = std::make_shared<services::InventoryService>(inventoryRepository_, eventPublisher_);
    
    // Initialize event handlers
    productEventHandler_ = std::make_shared<handlers::ProductEventHandler>(db);
    
    // Initialize event consumer (for receiving product events)
    utils::Logger::info("Initializing event consumer...");
    try {
        std::vector<std::string> routingKeys = {
            "product.created",
            "product.updated",
            "product.deleted"
        };
        
        eventConsumer_ = warehouse::messaging::EventConsumer::create("inventory-service", routingKeys);
        
        // Register event handlers (simple lambda that routes to ProductEventHandler)
        eventConsumer_->onEvent("product.created", [this](const warehouse::messaging::Event& event) {
            try {
                utils::Logger::debug("Received product.created event (id: {})", event.getId());
                productEventHandler_->handleProductCreated(event.getData());
            } catch (const std::exception& e) {
                utils::Logger::error("Error processing product.created event: {}", e.what());
                throw;  // Re-throw to trigger library retry logic
            }
        });
        
        eventConsumer_->onEvent("product.updated", [this](const warehouse::messaging::Event& event) {
            try {
                utils::Logger::debug("Received product.updated event (id: {})", event.getId());
                productEventHandler_->handleProductUpdated(event.getData());
            } catch (const std::exception& e) {
                utils::Logger::error("Error processing product.updated event: {}", e.what());
                throw;  // Re-throw to trigger library retry logic
            }
        });
        
        eventConsumer_->onEvent("product.deleted", [this](const warehouse::messaging::Event& event) {
            try {
                utils::Logger::debug("Received product.deleted event (id: {})", event.getId());
                productEventHandler_->handleProductDeleted(event.getData());
            } catch (const std::exception& e) {
                utils::Logger::error("Error processing product.deleted event: {}", e.what());
                throw;  // Re-throw to trigger library retry logic
            }
        });
        
        // Register catch-all handler for metrics/logging
        eventConsumer_->onAnyEvent([](const warehouse::messaging::Event& event) {
            utils::Logger::info("Processing event: {} (id: {})", event.getType(), event.getId());
        });
        
        // Start consuming events
        eventConsumer_->start();
        
        utils::Logger::info("Event consumer started successfully");
    } catch (const std::exception& e) {
        utils::Logger::error("Failed to initialize event consumer: {}", e.what());
        utils::Logger::warn("Service will continue without event consumption");
    }
    
    utils::Logger::info("Services initialized");
}

} // namespace inventory

#include "inventory/Application.hpp"
#include "inventory/utils/Config.hpp"
#include "inventory/utils/Logger.hpp"
#include "inventory/utils/Database.hpp"
#include "inventory/utils/MessageBus.hpp"
#include "inventory/utils/RabbitMqMessageBus.hpp"
#include "inventory/Server.hpp"
#include <stdexcept>

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

    // Load message bus configuration (env overrides JSON where provided)
    messageBusConfig_.host = utils::Config::getEnv("RABBITMQ_HOST",
        utils::Config::getString("messageBus.host", "rabbitmq"));
    messageBusConfig_.port = utils::Config::getInt("messageBus.port", 5672);
    messageBusConfig_.virtual_host = utils::Config::getEnv("RABBITMQ_VHOST",
        utils::Config::getString("messageBus.virtualHost", "/"));
    messageBusConfig_.username = utils::Config::getEnv("RABBITMQ_USER",
        utils::Config::getString("messageBus.username", "warehouse"));
    messageBusConfig_.password = utils::Config::getEnv("RABBITMQ_PASSWORD",
        utils::Config::getString("messageBus.password", "warehouse_dev"));
    messageBusConfig_.exchange = utils::Config::getString("messageBus.exchange", "warehouse.events");
    messageBusConfig_.routing_key_prefix = utils::Config::getString("messageBus.routingKeyPrefix", "inventory.");
    
    utils::Logger::info("Configuration loaded from {}", configPath);
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

    // Initialize message bus
    utils::Logger::info("Initializing RabbitMQ message bus...");
    messageBus_ = std::make_shared<utils::RabbitMqMessageBus>(messageBusConfig_);

    // Initialize services (message bus may be null if initialization failed)
    inventoryService_ = std::make_shared<services::InventoryService>(inventoryRepository_, messageBus_);
    
    utils::Logger::info("Services initialized");
}

} // namespace inventory

#include "inventory/Application.hpp"
#include "inventory/utils/Config.hpp"
#include "inventory/utils/Logger.hpp"
#include "inventory/utils/Database.hpp"
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
    
    // Initialize services
    inventoryService_ = std::make_shared<services::InventoryService>(inventoryRepository_);
    
    utils::Logger::info("Services initialized");
}

} // namespace inventory

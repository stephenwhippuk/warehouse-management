#include "warehouse/Application.hpp"
#include <thread>
#include <chrono>

namespace warehouse {

Application* Application::instance_ = nullptr;

Application::Application() {
    instance_ = this;
}

Application::~Application() {
    shutdown();
}

Application& Application::instance() {
    static Application app;
    return app;
}

bool Application::initialize(const std::string& configFile) {
    if (initialized_) {
        return true;
    }
    
    utils::Logger::info("Initializing application with config: {}", configFile);
    
    if (!loadConfiguration(configFile)) {
        utils::Logger::error("Failed to load configuration");
        return false;
    }
    
    if (!initializeDatabase()) {
        utils::Logger::error("Failed to initialize database");
        return false;
    }
    
    if (!initializeServices()) {
        utils::Logger::error("Failed to initialize services");
        return false;
    }
    
    if (!initializeServer()) {
        utils::Logger::error("Failed to initialize server");
        return false;
    }
    
    initialized_ = true;
    return true;
}

void Application::run() {
    if (!initialized_) {
        throw std::runtime_error("Application not initialized");
    }
    
    running_ = true;
    server_->start();
    
    // Keep running until shutdown is called
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Application::shutdown() {
    if (!running_) {
        return;
    }
    
    utils::Logger::info("Shutting down application...");
    
    running_ = false;
    
    if (server_) {
        server_->stop();
    }
    
    if (database_) {
        database_->disconnect();
    }
    
    utils::Logger::info("Application shutdown complete");
}

bool Application::loadConfiguration(const std::string& configFile) {
    auto& config = utils::Config::instance();
    
    // Try to load from file first
    if (!config.load(configFile)) {
        utils::Logger::warn("Could not load config file: {}, using defaults", configFile);
    }
    
    // Override with environment variables
    config.setFromEnv("server.host", "WAREHOUSE_HOST");
    config.setFromEnv("server.port", "WAREHOUSE_PORT");
    config.setFromEnv("database.host", "DB_HOST");
    config.setFromEnv("database.port", "DB_PORT");
    config.setFromEnv("database.user", "DB_USER");
    config.setFromEnv("database.password", "DB_PASSWORD");
    config.setFromEnv("database.database", "DB_NAME");
    
    return true;
}

bool Application::initializeDatabase() {
    auto& config = utils::Config::instance();
    auto dbConfig = config.getDatabaseConfig();
    
    database_ = std::make_shared<utils::Database>(
        utils::Database::Config{
            .host = dbConfig.host,
            .port = dbConfig.port,
            .database = dbConfig.database,
            .user = dbConfig.user,
            .password = dbConfig.password,
            .maxConnections = dbConfig.maxConnections
        }
    );
    
    if (!database_->connect()) {
        return false;
    }
    
    utils::Logger::info("Database connection established");
    return true;
}

bool Application::initializeServices() {
    // TODO: Initialize repositories and services
    // This will be implemented when the repository/service implementations are done
    utils::Logger::info("Services initialized");
    return true;
}

bool Application::initializeServer() {
    auto& config = utils::Config::instance();
    auto serverConfig = config.getServerConfig();
    
    // TODO: Pass actual service instances when implemented
    server_ = std::make_unique<Server>(
        Server::Config{
            .host = serverConfig.host,
            .port = serverConfig.port,
            .maxThreads = serverConfig.maxThreads,
            .maxQueued = serverConfig.maxQueued
        },
        nullptr, // warehouseService
        nullptr  // locationService
    );
    
    utils::Logger::info("Server initialized on {}:{}", serverConfig.host, serverConfig.port);
    return true;
}

void Application::setupSignalHandlers() {
    // Signal handlers are set up in main.cpp
}

} // namespace warehouse

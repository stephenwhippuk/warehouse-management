#include "product/Application.hpp"
#include "product/utils/Logger.hpp"
#include "product/utils/Config.hpp"
#include "product/utils/Database.hpp"
#include "product/repositories/ProductRepository.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

namespace product {

// Global app instance for signal handling
Application* g_app = nullptr;

void signalHandler(int signal) {
    if (g_app) {
        g_app->run(0, nullptr);  // Will be caught by try block and exit
    }
}

Application::Application() {
    g_app = this;
}

int Application::run(int argc, char* argv[]) {
    try {
        initialize();
        start();
        
        // Keep running until interrupted
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        if (auto logger = utils::Logger::getLogger()) {
            logger->error("Fatal error: {}", e.what());
        }
        stop();
        return EXIT_FAILURE;
    }
}

void Application::initialize() {
    // Initialize logger
    utils::Logger::initialize("info");
    if (auto logger = utils::Logger::getLogger()) logger->info("Starting product-service v1.0.0");
    
    // Load configuration
    try {
        utils::Config::load("config/application.json");
    } catch (const std::exception& e) {
        if (auto logger = utils::Logger::getLogger()) logger->warn("Could not load config file: {}, using defaults", e.what());
    }
    
    // Get configuration values
    std::string dbUrl = utils::Config::getEnv("DATABASE_URL",
        utils::Config::getString("database.connectionString", 
            "postgresql://warehouse:warehouse@localhost:5432/warehouse_db"));
    
    // Connect to database
    if (auto logger = utils::Logger::getLogger()) logger->info("Connecting to database...");
    utils::Database::connect(dbUrl);
    if (auto logger = utils::Logger::getLogger()) logger->info("Database connected successfully");
    
    // Initialize repository and service
    auto repository = std::make_shared<repositories::ProductRepository>(
        utils::Database::getConnection()
    );
    
    productService_ = std::make_shared<services::ProductService>(repository);
    if (auto logger = utils::Logger::getLogger()) logger->info("Product service initialized");
    
    // Initialize server
    int port = utils::Config::getInt("server.port", 8082);
    std::string host = utils::Config::getString("server.host", "0.0.0.0");
    
    server_ = std::make_unique<Server>(port, host);
    if (auto logger = utils::Logger::getLogger()) logger->info("HTTP server configured on {}:{}", host, port);
}

void Application::start() {
    if (server_ && productService_) {
        server_->setService(productService_);
        server_->start();
        if (auto logger = utils::Logger::getLogger()) logger->info("HTTP server started");
    }
}

void Application::stop() {
    if (auto logger = utils::Logger::getLogger()) logger->info("Shutting down...");
    
    if (server_) {
        server_->stop();
    }
    
    utils::Database::disconnect();
    if (auto logger = utils::Logger::getLogger()) logger->info("Shutdown complete");
}

}  // namespace product

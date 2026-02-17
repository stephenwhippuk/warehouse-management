#include "product/Application.hpp"
#include "product/utils/Logger.hpp"
#include "product/utils/Config.hpp"
#include "product/utils/Database.hpp"
#include "product/repositories/ProductRepository.hpp"
#include "product/services/IProductService.hpp"
#include "product/services/ProductService.hpp"
#include "product/controllers/ProductController.hpp"
#include "product/controllers/HealthController.hpp"
#include "contract-plugin/ContractPlugin.hpp"
#include "http-framework/ServiceCollection.hpp"
#include "http-framework/HttpHost.hpp"
#include "http-framework/IPlugin.hpp"
#include <warehouse/messaging/EventPublisher.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <functional>
#include <memory>
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
    
    // Database will be created and connected via DI container
    if (auto logger = utils::Logger::getLogger()) logger->info("Database will be initialized via DI container...");
    
    // =========================================================================
    // Setup Dependency Injection Container
    // =========================================================================
    if (auto logger = utils::Logger::getLogger()) logger->info("Configuring DI container...");
    http::ServiceCollection services;
    
    // Singleton services
    services.addService<utils::Database>([dbUrl](http::IServiceProvider& /* provider */) {
        if (auto logger = utils::Logger::getLogger()) logger->info("Creating Database (Singleton)");
        
        // Parse connection string to extract components
        // Format: postgresql://user:pass@host:port/dbname
        utils::Database::Config dbConfig;
        
        // Simple parsing (TODO: Make more robust)
        size_t slashPos = dbUrl.rfind('/');
        if (slashPos != std::string::npos) {
            dbConfig.database = dbUrl.substr(slashPos + 1);
        }
        
        auto db = std::make_shared<utils::Database>(dbConfig);
        if (!db->connect()) {
            throw std::runtime_error("Failed to connect to database");
        }
        return db;
    }, http::ServiceLifetime::Singleton);
    
    services.addService<warehouse::messaging::EventPublisher>([](http::IServiceProvider& provider) {
        if (auto logger = utils::Logger::getLogger()) logger->info("Creating EventPublisher (Singleton)");
        return std::shared_ptr<warehouse::messaging::EventPublisher>(
            warehouse::messaging::EventPublisher::create("product-service")
        );
    }, http::ServiceLifetime::Singleton);
    
    // Scoped services (per-request)
    services.addScoped<repositories::ProductRepository, repositories::ProductRepository>();
    
    services.addScoped<services::IProductService, services::ProductService>();
    
    // =========================================================================
    // Register Contract Plugin (Claims + Swagger)
    // =========================================================================
    contract::ContractConfig contractConfig = contract::ContractConfig::fromEnvironment();
    contractConfig.claimsPath = utils::Config::getString("contracts.claimsPath", "claims.json");
    contractConfig.contractsPath = utils::Config::getString("contracts.contractsPath", "contracts");
    contractConfig.globalContractsPath = utils::Config::getString("contracts.globalContractsPath", "../../contracts");
    contractConfig.enableClaims = utils::Config::getBool("contracts.enableClaims", true);
    contractConfig.enableSwagger = utils::Config::getBool("contracts.enableSwagger", true);
    contractConfig.enableValidation = utils::Config::getBool("contracts.enableValidation", false);

    contractConfig.swaggerTitle = utils::Config::getString("service.name", "product-service") + " API";
    contractConfig.swaggerVersion = utils::Config::getString("service.version", "1.0.0");
    contractConfig.swaggerDescription = "Product master data management service";

    contractPlugin_ = std::make_shared<contract::ContractPlugin>(contractConfig);
    http::HttpHost::registerPlugin(services, *contractPlugin_);

    // Build service provider
    serviceProvider_ = services.buildServiceProvider();
    if (auto logger = utils::Logger::getLogger()) logger->info("DI container configured");
    
    // =========================================================================
    // Setup HTTP Host
    // =========================================================================
    int port = utils::Config::getInt("server.port", 8082);
    std::string host = utils::Config::getString("server.host", "0.0.0.0");
    
    httpHost_ = std::make_unique<http::HttpHost>(port, serviceProvider_, host);

    if (contractPlugin_) {
        httpHost_->usePlugin(*contractPlugin_, *serviceProvider_);
    }

    // TODO: Add other middleware (Logging, Auth, CORS) here
    
    // Register controllers
    httpHost_->addController(std::make_shared<controllers::ProductController>());
    httpHost_->addController(std::make_shared<controllers::HealthController>());

    
    if (auto logger = utils::Logger::getLogger()) {
        logger->info("HTTP host configured on {}:{}", host, port);
    }
}

void Application::start() {
    if (httpHost_) {
        if (auto logger = utils::Logger::getLogger()) logger->info("Starting HTTP host...");
        httpHost_->start();
        if (auto logger = utils::Logger::getLogger()) logger->info("HTTP host started");
    }
}

void Application::stop() {
    if (auto logger = utils::Logger::getLogger()) logger->info("Shutting down...");
    
    if (httpHost_) {
        // HttpHost will stop automatically on destruction
    }
    
    // Database will be cleaned up by DI container
    if (auto logger = utils::Logger::getLogger()) logger->info("Shutdown complete");
}

}  // namespace product

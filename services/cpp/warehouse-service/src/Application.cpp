#include "warehouse/Application.hpp"
#include "warehouse/utils/Logger.hpp"
#include "warehouse/utils/Config.hpp"
#include "warehouse/utils/Database.hpp"
#include "warehouse/repositories/WarehouseRepository.hpp"
#include "warehouse/repositories/LocationRepository.hpp"
#include "warehouse/services/IWarehouseService.hpp"
#include "warehouse/services/ILocationService.hpp"
#include "warehouse/services/WarehouseService.hpp"
#include "warehouse/services/LocationService.hpp"
#include "warehouse/controllers/WarehouseController.hpp"
#include "warehouse/controllers/LocationController.hpp"
#include "warehouse/controllers/HealthController.hpp"
#include "http-framework/ServiceCollection.hpp"
#include "http-framework/HttpHost.hpp"
#include "http-framework/ServiceScopeMiddleware.hpp"
#include "http-framework/Middleware.hpp"
#include "contract-plugin/ContractPlugin.hpp"
#include <warehouse/messaging/EventPublisher.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <functional>

namespace warehouse {

// Global app instance for signal handling
Application* g_app = nullptr;

void signalHandler(int /* signal */) {
    if (g_app) {
        g_app->stop();  // Gracefully shutdown on signal
    }
}

Application::Application() {
    g_app = this;
}

int Application::run(int /* argc */, char* /* argv */[]) {
    try {
        initialize();
        start();
        
        // Keep running until interrupted
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        utils::Logger::error("Fatal error: {}", e.what());
        stop();
        return EXIT_FAILURE;
    }
}

void Application::initialize() {
    // Initialize logger
    utils::Logger::init("", utils::Logger::Level::Info, true);
    utils::Logger::info("Starting warehouse-service v1.0.0");
    
    // Optionally load configuration (skip if file doesn't exist)
    // Config is not used directly, we use environment variables instead
    
    // =========================================================================
    // Setup Dependency Injection Container
    // =========================================================================
    utils::Logger::info("Configuring DI container...");
    http::ServiceCollection services;
    
    // Singleton services
    services.addService<utils::Database>([](http::IServiceProvider& /* provider */) {
        utils::Logger::info("Creating Database singleton");
        
        // Get database URL from environment or use default
        const char* dbUrlEnv = std::getenv("DATABASE_URL");
        std::string dbUrl = dbUrlEnv ? dbUrlEnv : 
            "postgresql://warehouse:warehouse@localhost:5432/warehouse_db";
        
        utils::Database::Config dbConfig;
        // TODO: Parse connection string to config properly
        // For now, using simplified approach
        auto db = std::make_shared<utils::Database>(dbConfig);
        db->connect();
        return db;
    }, http::ServiceLifetime::Singleton);
    
    services.addService<warehouse::messaging::EventPublisher>([](http::IServiceProvider& /* provider */) {
        utils::Logger::info("Creating EventPublisher (Singleton)");
        return std::shared_ptr<warehouse::messaging::EventPublisher>(
            warehouse::messaging::EventPublisher::create("warehouse-service")
        );
    }, http::ServiceLifetime::Singleton);
    
    // Scoped services (per-request)
    services.addScoped<repositories::WarehouseRepository, repositories::WarehouseRepository>();
    
    services.addScoped<repositories::LocationRepository, repositories::LocationRepository>();
    
    services.addScoped<services::IWarehouseService, services::WarehouseService>();
    
    services.addScoped<services::ILocationService, services::LocationService>();
    
    // =========================================================================
    // Register Contract Plugin (Claims + Swagger)
    // =========================================================================
    contract::ContractConfig contractConfig = contract::ContractConfig::fromEnvironment();
    contractConfig.claimsPath = utils::Config::instance().getString("contracts.claimsPath", "claims.json");
    contractConfig.contractsPath = utils::Config::instance().getString("contracts.contractsPath", "contracts");
    contractConfig.globalContractsPath = utils::Config::instance().getString("contracts.globalContractsPath", "../../contracts");
    contractConfig.enableClaims = utils::Config::instance().getBool("contracts.enableClaims", true);
    contractConfig.enableSwagger = utils::Config::instance().getBool("contracts.enableSwagger", true);
    contractConfig.enableValidation = utils::Config::instance().getBool("contracts.enableValidation", false);

    contractConfig.swaggerTitle = utils::Config::instance().getString("service.name", "warehouse-service") + " API";
    contractConfig.swaggerVersion = utils::Config::instance().getString("service.version", "1.0.0");
    contractConfig.swaggerDescription = "Warehouse and location management service";

    contract::ContractPlugin contractPlugin(contractConfig);
    http::HttpHost::registerPlugin(services, contractPlugin);
    
    // Build service provider
    serviceProvider_ = services.buildServiceProvider();
    utils::Logger::info("DI container configured");
    
    // =========================================================================
    // Setup HTTP Host
    // =========================================================================
    // Get port from environment or use default
    const char* portEnv = std::getenv("SERVER_PORT");
    int port = portEnv ? std::stoi(portEnv) : 8083;
    std::string host = "0.0.0.0";
    
    httpHost_ = std::make_unique<http::HttpHost>(port, serviceProvider_, host);

    httpHost_->usePlugin(contractPlugin, *serviceProvider_);

    // TODO: Add other middleware (Logging, Auth, CORS) here
    
    // Register controllers
    httpHost_->addController(std::make_shared<controllers::WarehouseController>());
    httpHost_->addController(std::make_shared<controllers::LocationController>());
    httpHost_->addController(std::make_shared<controllers::HealthController>());
    
    utils::Logger::info("HTTP host configured on {}:{}", host, port);
}

void Application::start() {
    if (httpHost_) {
        utils::Logger::info("Starting HTTP host...");
        httpHost_->start();
        utils::Logger::info("HTTP host started");
    }
}

void Application::stop() {
    utils::Logger::info("Shutting down...");
    
    if (httpHost_) {
        // HttpHost will stop automatically on destruction
    }
    
    // Database singleton will be destroyed automatically
    utils::Logger::info("Shutdown complete");
}

}  // namespace warehouse

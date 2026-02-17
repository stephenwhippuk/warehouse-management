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
        
        utils::Database::Config dbConfig;
        
        // UNIFIED CONFIG: Primary source is DATABASE_URL (PostgreSQL standard)
        // Format: postgresql://user:pass@host:port/dbname
        const char* dbUrl = std::getenv("DATABASE_URL");
        
        if (dbUrl && std::strlen(dbUrl) > 0) {
            // Parse DATABASE_URL connection string
            std::string url(dbUrl);
            utils::Logger::info("Using DATABASE_URL for configuration");
            
            // Simple parser for postgresql://user:pass@host:port/dbname
            size_t protocolPos = url.find("://");
            if (protocolPos != std::string::npos) {
                size_t authStart = protocolPos + 3;
                size_t atPos = url.find('@', authStart);
                
                if (atPos != std::string::npos) {
                    // Extract user:pass
                    std::string auth = url.substr(authStart, atPos - authStart);
                    size_t colonPos = auth.find(':');
                    if (colonPos != std::string::npos) {
                        dbConfig.user = auth.substr(0, colonPos);
                        dbConfig.password = auth.substr(colonPos + 1);
                    }
                    
                    // Extract host:port/dbname
                    size_t hostStart = atPos + 1;
                    size_t slashPos = url.find('/', hostStart);
                    if (slashPos != std::string::npos) {
                        std::string hostPort = url.substr(hostStart, slashPos - hostStart);
                        size_t portPos = hostPort.find(':');
                        if (portPos != std::string::npos) {
                            dbConfig.host = hostPort.substr(0, portPos);
                            dbConfig.port = std::stoi(hostPort.substr(portPos + 1));
                        } else {
                            dbConfig.host = hostPort;
                            dbConfig.port = 5432; // Default PostgreSQL port
                        }
                        
                        // Extract database name
                        dbConfig.database = url.substr(slashPos + 1);
                    }
                }
            }
        } else {
            // Fallback to individual environment variables for flexibility
            utils::Logger::info("DATABASE_URL not set, using individual environment variables");
            
            const char* dbHost = std::getenv("DATABASE_HOST");
            dbConfig.host = dbHost ? dbHost : "localhost";
            
            const char* dbPort = std::getenv("DATABASE_PORT");
            dbConfig.port = dbPort ? std::stoi(dbPort) : 5432;
            
            const char* dbName = std::getenv("DATABASE_NAME");
            dbConfig.database = dbName ? dbName : "warehouse_db";
            
            const char* dbUser = std::getenv("DATABASE_USER");
            dbConfig.user = dbUser ? dbUser : "warehouse";
            
            const char* dbPassword = std::getenv("DATABASE_PASSWORD");
            dbConfig.password = dbPassword ? dbPassword : "warehouse";
        }
        
        utils::Logger::info("Database config - host={}, port={}, db={}, user={}",
            dbConfig.host, dbConfig.port, dbConfig.database, dbConfig.user);
        
        auto db = std::make_shared<utils::Database>(dbConfig);
        if (!db->connect()) {
            utils::Logger::error("Failed to connect to database!");
            // Don't throw, let the service return the failed db object
            // Error will be caught when methods try to use it
        } else {
            utils::Logger::info("Database connection successful");
        }
        return db;
    }, http::ServiceLifetime::Singleton);
    
    services.addService<warehouse::messaging::EventPublisher>([](http::IServiceProvider& /* provider */) {
        utils::Logger::info("Creating EventPublisher (Singleton)");
        try {
            return std::shared_ptr<warehouse::messaging::EventPublisher>(
                warehouse::messaging::EventPublisher::create("warehouse-service")
            );
        } catch (const std::exception& e) {
            utils::Logger::error("Failed to create EventPublisher: {}", e.what());
            utils::Logger::warn("Service will continue without event publishing capability");
            return std::shared_ptr<warehouse::messaging::EventPublisher>(nullptr);
        }
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

    contractPlugin_ = std::make_shared<contract::ContractPlugin>(contractConfig);
    http::HttpHost::registerPlugin(services, *contractPlugin_);
    
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

    // NOTE: ServiceScopeMiddleware is automatically added by HttpHost constructor
    // Do NOT manually add it again or it will be duplicated!
    
    // Add plugin middleware and controllers (this adds ContractValidationMiddleware)
    if (contractPlugin_) {
        httpHost_->usePlugin(*contractPlugin_, *serviceProvider_);
    }
    
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

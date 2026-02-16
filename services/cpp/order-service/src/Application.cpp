#include "order/Application.hpp"
#include "order/utils/Logger.hpp"
#include "order/utils/Config.hpp"
#include "order/utils/Database.hpp"
#include "order/repositories/OrderRepository.hpp"
#include "order/services/OrderService.hpp"
#include "order/services/IOrderService.hpp"
#include "order/controllers/OrderController.hpp"
#include "order/controllers/HealthController.hpp"
#include <http-framework/ServiceCollection.hpp>
#include <http-framework/ServiceScopeMiddleware.hpp>
#include <http-framework/Middleware.hpp>
#include "contract-plugin/ContractPlugin.hpp"
#include <warehouse/messaging/EventPublisher.hpp>
#include <warehouse/messaging/EventConsumer.hpp>
#include <cstdlib>

namespace order {

Application::Application() = default;

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    try {
        initializeLogger();
        initializeConfig();
        initializeDI();
        initializeHttpServer();
        // initializeMessaging();  // TODO: Add when ready
        
        return true;
    } catch (const std::exception& e) {
        utils::Logger::critical("Failed to initialize application: {}", e.what());
        return false;
    }
}

void Application::initializeLogger() {
    utils::Logger::init("logs/order-service.log",
                       utils::Logger::Level::Info,
                       true);
    utils::Logger::info("Initializing Order Service...");
}

void Application::initializeConfig() {
    // Load config file
    auto& config = utils::Config::instance();
    if (!config.load("config/application.json")) {
        utils::Logger::warn("Failed to load config file, using defaults");
    }
    
    // Override with environment variables
    const char* portEnv = std::getenv("SERVER_PORT");
    if (portEnv) {
        serverPort_ = std::atoi(portEnv);
    } else {
        serverPort_ = config.getInt("server.port", 8083);
    }
    
    const char* hostEnv = std::getenv("SERVER_HOST");
    if (hostEnv) {
        serverHost_ = hostEnv;
    } else {
        serverHost_ = config.getString("server.host", "0.0.0.0");
    }
    
    const char* dbUrlEnv = std::getenv("DATABASE_URL");
    if (dbUrlEnv) {
        dbConnectionString_ = dbUrlEnv;
    } else {
        dbConnectionString_ = config.getString("database.connectionString",
            "postgresql://order:order_dev@localhost:5432/order_db");
    }
    
    utils::Logger::info("Server configuration: {}:{}", serverHost_, serverPort_);
    utils::Logger::info("Database: {}", dbConnectionString_.substr(0, dbConnectionString_.find('@')));
}

void Application::initializeDI() {
    utils::Logger::info("Initializing DI container...");
    
    http::ServiceCollection services;
    
    // =========================================================================
    // CRITICAL: Register Database as Singleton
    // =========================================================================
    auto dbConnStr = dbConnectionString_;
    services.addService<utils::Database>(
        [dbConnStr](http::IServiceProvider& /* provider */) -> std::shared_ptr<utils::Database> {
            utils::Logger::info("Creating Database singleton");
            
            // Parse connection string to Config
            utils::Database::Config dbConfig;
            
            // Simple parser: postgresql://user:pass@host:port/database
            size_t atPos = dbConnStr.find('@');
            size_t slashPos = dbConnStr.rfind('/');
            
            if (slashPos != std::string::npos) {
                dbConfig.database = dbConnStr.substr(slashPos + 1);
            }
            
            // TODO: Parse host, port, user, password more robustly
            
            auto db = std::make_shared<utils::Database>(dbConfig);
            if (!db->connect()) {
                throw std::runtime_error("Failed to connect to database");
            }
            return db;
        },
        http::ServiceLifetime::Singleton
    );
    
    // Register repositories (Scoped - per request)
    services.addScoped<repositories::OrderRepository, repositories::OrderRepository>();
    
    // Register services (Scoped - per request)
    services.addScoped<services::IOrderService, services::OrderService>();
    
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

    contractConfig.swaggerTitle = utils::Config::instance().getString("service.name", "order-service") + " API";
    contractConfig.swaggerVersion = utils::Config::instance().getString("service.version", "1.0.0");
    contractConfig.swaggerDescription = "Order management and fulfillment service";

    contract::ContractPlugin contractPlugin(contractConfig);
    http::HttpHost::registerPlugin(services, contractPlugin);
    
    // Build service provider
    serviceProvider_ = services.buildServiceProvider();
    
    utils::Logger::info("DI container initialized");
}

void Application::initializeHttpServer() {
    utils::Logger::info("Initializing HTTP server...");
    
    contract::ContractConfig contractConfig = contract::ContractConfig::fromEnvironment();
    contractConfig.claimsPath = utils::Config::instance().getString("contracts.claimsPath", "claims.json");
    contractConfig.contractsPath = utils::Config::instance().getString("contracts.contractsPath", "contracts");
    contractConfig.globalContractsPath = utils::Config::instance().getString("contracts.globalContractsPath", "../../contracts");
    contractConfig.enableClaims = utils::Config::instance().getBool("contracts.enableClaims", true);
    contractConfig.enableSwagger = utils::Config::instance().getBool("contracts.enableSwagger", true);
    contractConfig.enableValidation = utils::Config::instance().getBool("contracts.enableValidation", false);

    contractConfig.swaggerTitle = utils::Config::instance().getString("service.name", "order-service") + " API";
    contractConfig.swaggerVersion = utils::Config::instance().getString("service.version", "1.0.0");
    contractConfig.swaggerDescription = "Order management and fulfillment service";
    
    contract::ContractPlugin contractPlugin(contractConfig);
    
    httpHost_ = std::make_unique<http::HttpHost>(serverPort_, serviceProvider_, serverHost_);
    
    httpHost_->usePlugin(contractPlugin, *serviceProvider_);
    
    // TODO: Add other middleware (logging, auth, cors)
    
    // Register controllers
    httpHost_->addController(std::make_shared<controllers::OrderController>(*serviceProvider_));
    httpHost_->addController(std::make_shared<controllers::HealthController>(*serviceProvider_));
    
    utils::Logger::info("HTTP server initialized");
}

void Application::initializeMessaging() {
    // TODO: Initialize EventPublisher and EventConsumer
    utils::Logger::info("Messaging initialization skipped (not implemented yet)");
}

void Application::run() {
    utils::Logger::info("Starting HTTP server on {}:{}...", serverHost_, serverPort_);
    httpHost_->start();
    utils::Logger::info("Order Service is running. Press Ctrl+C to stop.");
}

void Application::shutdown() {
    utils::Logger::info("Shutting down Order Service...");
    
    if (httpHost_) {
        httpHost_->stop();
        httpHost_.reset();
    }
    
    // Database cleanup handled automatically by DI container
    // DO NOT call Database::disconnect() manually
    
    utils::Logger::info("Order Service stopped");
}

} // namespace order

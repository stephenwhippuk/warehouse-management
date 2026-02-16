#include "inventory/Application.hpp"
#include "inventory/utils/Config.hpp"
#include "inventory/utils/Logger.hpp"
#include "inventory/utils/Database.hpp"
#include "inventory/repositories/InventoryRepository.hpp"
#include "inventory/services/IInventoryService.hpp"
#include "inventory/services/InventoryService.hpp"
#include "inventory/controllers/InventoryController.hpp"
#include "inventory/controllers/HealthController.hpp"
#include "contract-plugin/ContractPlugin.hpp"
#include <warehouse/messaging/EventConsumer.hpp>
#include <warehouse/messaging/EventPublisher.hpp>
#include <http-framework/ServiceCollection.hpp>
#include <http-framework/HttpHost.hpp>
#include <http-framework/IServiceScope.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <csignal>
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace inventory {

// Global app instance for signal handling
Application* g_app = nullptr;

void signalHandler(int /* signal */) {
    if (g_app) {
        g_app->shutdown();  // Gracefully shutdown on signal
    }
}

Application::Application() : initialized_(false), serverPort_(8080) {
    g_app = this;
}

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
    
    // =========================================================================
    // Setup HTTP Host with Contract Plugin
    // =========================================================================
    httpHost_ = std::make_unique<http::HttpHost>(serverPort_, serviceProvider_, "0.0.0.0");
    
    // Apply contract plugin (claims + swagger)
    if (contractPlugin_) {
        httpHost_->usePlugin(*contractPlugin_, *serviceProvider_);
    }
    
    // Add additional middleware
    httpHost_->use(std::make_shared<http::LoggingMiddleware>());
    httpHost_->use(std::make_shared<http::CorsMiddleware>());
    
    // Register controllers
    httpHost_->addController(std::make_shared<controllers::HealthController>());
    httpHost_->addController(std::make_shared<controllers::InventoryController>());
    
    // Configure server
    httpHost_->setMaxThreads(16);
    httpHost_->setMaxQueued(100);
    httpHost_->setTimeout(60);
    
    // Start the server
    httpHost_->start();
    utils::Logger::info("HTTP Server started on port {}", serverPort_);
    
    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Keep running until interrupted
    while (initialized_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    utils::Logger::info("Inventory Service stopped");
}

void Application::shutdown() {
    if (!initialized_) {
        return;
    }
    
    utils::Logger::info("Shutting down Inventory Service");
    
    // Stop HTTP host
    if (httpHost_) {
        httpHost_->stop();
        httpHost_.reset();
    }
    
    // Stop event consumer gracefully
    if (eventConsumer_ && eventConsumer_->isRunning()) {
        eventConsumer_->stop();
    }
    
    // Database will be cleaned up by DI container
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
    // Database will be created and connected via DI container
    utils::Logger::info("Database will be initialized via DI container...");
}

void Application::initializeServices() {
    utils::Logger::info("Initializing dependency injection container...");
    
    http::ServiceCollection services;
    
    // Register database as singleton
    auto dbConnStr = dbConnectionString_;
    services.addService<utils::Database>(
        [dbConnStr](http::IServiceProvider& /* provider */) -> std::shared_ptr<utils::Database> {
            utils::Logger::info("Creating Database singleton");
            
            // Parse connection string to extract components
            // Format: postgresql://user:pass@host:port/dbname
            utils::Database::Config dbConfig;
            
            // Simple parsing (TODO: Make more robust)
            size_t slashPos = dbConnStr.rfind('/');
            if (slashPos != std::string::npos) {
                dbConfig.database = dbConnStr.substr(slashPos + 1);
            }
            
            auto db = std::make_shared<utils::Database>(dbConfig);
            if (!db->connect()) {
                throw std::runtime_error("Failed to connect to database");
            }
            return db;
        },
        http::ServiceLifetime::Singleton
    );
    
    // Register event publisher as singleton
    services.addService<warehouse::messaging::EventPublisher>(
        [](http::IServiceProvider& provider) -> std::shared_ptr<warehouse::messaging::EventPublisher> {
            (void)provider;  // Unused
            return std::shared_ptr<warehouse::messaging::EventPublisher>(
                warehouse::messaging::EventPublisher::create("inventory-service")
            );
        },
        http::ServiceLifetime::Singleton
    );
    
    // Register repository as scoped (per-request reuse)
    services.addScoped<repositories::InventoryRepository, repositories::InventoryRepository>();
    
    // Register service as scoped (per-request reuse)
    services.addScoped<services::IInventoryService, services::InventoryService>();
    
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

    contractConfig.swaggerTitle = utils::Config::getString("service.name", "inventory-service") + " API";
    contractConfig.swaggerVersion = utils::Config::getString("service.version", "1.0.0");
    contractConfig.swaggerDescription = "Inventory allocation and fulfillment service";

    contractPlugin_ = std::make_shared<contract::ContractPlugin>(contractConfig);
    http::HttpHost::registerPlugin(services, *contractPlugin_);
    
    // Build service provider
    serviceProvider_ = services.buildServiceProvider();
    
    // Get event publisher for event consumer setup
    auto scope = serviceProvider_->createScope();
    eventPublisher_ = scope->getServiceProvider().getService<warehouse::messaging::EventPublisher>();
    
    // Initialize event handlers
    auto db = scope->getServiceProvider().getService<utils::Database>();
    productEventHandler_ = std::make_shared<handlers::ProductEventHandler>(db->getConnection());
    
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

#include "inventory/Server.hpp"
#include "inventory/controllers/InventoryController.hpp"
#include "inventory/controllers/HealthController.hpp"
#include "inventory/controllers/ClaimsController.hpp"
#include "inventory/utils/Logger.hpp"
#include <http-framework/Middleware.hpp>
#include <http-framework/ServiceScopeMiddleware.hpp>
#include <atomic>
#include <csignal>
#include <chrono>
#include <thread>

namespace inventory {

namespace {

std::atomic_bool g_running{true};

void signalHandler(int) {
    g_running = false;
}

void waitForTerminationRequest() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace

Server::Server(int port) : port_(port) {}

Server::~Server() {
    stop();
}

void Server::setServiceProvider(std::shared_ptr<http::IServiceProvider> provider) {
    serviceProvider_ = provider;
}

void Server::start() {
    httpHost_ = std::make_unique<http::HttpHost>(port_);
    
    // Add middleware - ServiceScopeMiddleware FIRST so scope is available to all others
    httpHost_->use(std::make_shared<http::ServiceScopeMiddleware>(serviceProvider_));
    httpHost_->use(std::make_shared<http::LoggingMiddleware>());
    httpHost_->use(std::make_shared<http::CorsMiddleware>());
    httpHost_->use(std::make_shared<http::ErrorHandlingMiddleware>());
    
    // Register controllers
    httpHost_->addController(std::make_shared<controllers::HealthController>());
    httpHost_->addController(std::make_shared<controllers::ClaimsController>());
    httpHost_->addController(std::make_shared<controllers::InventoryController>());
    
    // Configure server
    httpHost_->setMaxThreads(16);
    httpHost_->setMaxQueued(100);
    httpHost_->setTimeout(60);
    
    // Start the server
    httpHost_->start();
    utils::Logger::info("HTTP Server started on port {}", port_);
    
    // Wait for termination signal
    utils::Logger::info("Press Ctrl+C to stop the server");
    waitForTerminationRequest();
    
    stop();
}

void Server::stop() {
    if (httpHost_) {
        utils::Logger::info("Stopping HTTP server...");
        httpHost_->stop();
        httpHost_.reset();
    }
}

} // namespace inventory

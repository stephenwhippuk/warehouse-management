#include "warehouse/Server.hpp"
#include "warehouse/controllers/WarehouseController.hpp"
#include "warehouse/controllers/LocationController.hpp"
#include "warehouse/controllers/HealthController.hpp"
#include "warehouse/controllers/SwaggerController.hpp"
#include "warehouse/controllers/ClaimsController.hpp"
#include "warehouse/utils/Logger.hpp"
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>

namespace warehouse {

class Server::RequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    RequestHandlerFactory(std::shared_ptr<services::WarehouseService> warehouseService,
                         std::shared_ptr<services::LocationService> locationService)
        : warehouseService_(warehouseService)
        , locationService_(locationService) {}
    
    Poco::Net::HTTPRequestHandler* createRequestHandler(
        const Poco::Net::HTTPServerRequest& request) override {
        
        const std::string& uri = request.getURI();
        
        // Health check endpoint
        if (uri == "/health") {
            return new controllers::HealthController();
        }
        
        // Swagger/OpenAPI endpoint
        if (uri == "/api/swagger.json") {
            return new controllers::SwaggerController();
        }
        
        // Claims endpoints
        if (uri.find("/api/v1/claims") == 0) {
            return new controllers::ClaimsController();
        }
        
        // Route to appropriate controller
        if (uri.find("/api/v1/warehouses") == 0) {
            return new controllers::WarehouseController(warehouseService_);
        } else if (uri.find("/api/v1/locations") == 0) {
            return new controllers::LocationController(locationService_);
        }
        
        return nullptr; // Will result in 404
    }

private:
    std::shared_ptr<services::WarehouseService> warehouseService_;
    std::shared_ptr<services::LocationService> locationService_;
};

Server::Server(const Config& config,
               std::shared_ptr<services::WarehouseService> warehouseService,
               std::shared_ptr<services::LocationService> locationService)
    : config_(config)
    , warehouseService_(warehouseService)
    , locationService_(locationService) {}

Server::~Server() {
    stop();
}

void Server::start() {
    if (running_) {
        return;
    }
    
    try {
        Poco::Net::ServerSocket socket(config_.port);
        
        auto params = new Poco::Net::HTTPServerParams;
        params->setMaxThreads(config_.maxThreads);
        params->setMaxQueued(config_.maxQueued);
        
        httpServer_ = std::make_unique<Poco::Net::HTTPServer>(
            new RequestHandlerFactory(warehouseService_, locationService_),
            socket,
            params
        );
        
        httpServer_->start();
        running_ = true;
        
        utils::Logger::info("HTTP Server started on {}:{}", config_.host, config_.port);
    } catch (const Poco::Exception& e) {
        utils::Logger::error("Failed to start server: {}", e.displayText());
        throw;
    }
}

void Server::stop() {
    if (!running_) {
        return;
    }
    
    utils::Logger::info("Stopping HTTP server...");
    
    if (httpServer_) {
        httpServer_->stop();
        httpServer_.reset();
    }
    
    running_ = false;
    utils::Logger::info("HTTP server stopped");
}

bool Server::isRunning() const {
    return running_;
}

} // namespace warehouse

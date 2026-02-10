#include "order/Server.hpp"
#include "order/controllers/OrderController.hpp"
#include "order/controllers/HealthController.hpp"
#include "order/controllers/ClaimsController.hpp"
#include "order/utils/Logger.hpp"
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>

namespace order {

class Server::RequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    RequestHandlerFactory(std::shared_ptr<services::OrderService> orderService)
        : orderService_(orderService) {}
    
    Poco::Net::HTTPRequestHandler* createRequestHandler(
        const Poco::Net::HTTPServerRequest& request) override {
        
        const std::string& uri = request.getURI();
        
        // Health check endpoint
        if (uri == "/health") {
            return new controllers::HealthController();
        }
        
        // Claims endpoints
        if (uri.find("/api/v1/claims") == 0) {
            return new controllers::ClaimsController();
        }
        
        // TODO: Add Swagger controller
        // if (uri == "/api/swagger.json") {
        //     return new controllers::SwaggerController();
        // }
        
        // Order endpoints
        if (uri.find("/api/v1/orders") == 0) {
            return new controllers::OrderController(orderService_);
        }
        
        return nullptr; // Will result in 404
    }

private:
    std::shared_ptr<services::OrderService> orderService_;
};

Server::Server(const Config& config, std::shared_ptr<services::OrderService> orderService)
    : config_(config)
    , orderService_(orderService) {}

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
            new RequestHandlerFactory(orderService_),
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

} // namespace order

#include "product/Server.hpp"
#include "product/utils/Logger.hpp"
#include "product/controllers/ProductController.hpp"
#include "product/controllers/HealthController.hpp"
#include "product/controllers/SwaggerController.hpp"
#include "product/services/ProductService.hpp"
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServer.h>

namespace product {

// Factory for creating request handlers based on URI
class RequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    explicit RequestHandlerFactory(std::shared_ptr<services::ProductService> service)
        : productService_(service) {}
    
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) override {
        std::string uri = request.getURI();
        std::string method = request.getMethod();
        
        // Route to appropriate handler
        if (uri == "/health") {
            return new controllers::HealthController();
        } else if (uri == "/api/swagger.json") {
            return new controllers::SwaggerController();
        } else if (uri.find("/api/v1/products") == 0) {
            return new controllers::ProductController(productService_);
        }
        
        // Default 404 handler
        return new controllers::ProductController(productService_);
    }

private:
    std::shared_ptr<services::ProductService> productService_;
};

Server::Server(int port, const std::string& host)
    : port_(port), host_(host) {
}

Server::~Server() {
    stop();
}

void Server::setService(std::shared_ptr<services::ProductService> service) {
    service_ = service;
}

void Server::start() {
    if (!service_) {
        if (auto logger = utils::Logger::getLogger()) {
            logger->error("Cannot start server: service not set");
        }
        return;
    }
    
    try {
        // Create socket with address and port
        Poco::Net::SocketAddress addr(host_, port_);
        Poco::Net::ServerSocket socket(addr);
        
        // Configure server parameters
        auto params = new Poco::Net::HTTPServerParams();
        params->setMaxThreads(4);
        params->setTimeout(Poco::Timespan(30, 0));  // 30 seconds
        
        auto factory = new RequestHandlerFactory(service_);
        
        // Create and start HTTP server
        httpServer_ = std::make_unique<Poco::Net::HTTPServer>(factory, socket, params);
        httpServer_->start();
        isRunning_ = true;
        
        if (auto logger = utils::Logger::getLogger()) {
            logger->info("HTTP server listening on {}:{}", host_, port_);
        }
    } catch (const std::exception& e) {
        if (auto logger = utils::Logger::getLogger()) {
            logger->error("Failed to start HTTP server: {}", e.what());
        }
        throw;
    }
}

void Server::stop() {
    if (httpServer_) {
        httpServer_->stop();
        isRunning_ = false;
        if (auto logger = utils::Logger::getLogger()) logger->info("HTTP server stopped");
    }
}

}  // namespace product

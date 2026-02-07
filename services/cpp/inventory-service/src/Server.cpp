#include "inventory/Server.hpp"
#include "inventory/controllers/InventoryController.hpp"
#include "inventory/utils/Logger.hpp"
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/ServerSocket.h>

namespace inventory {

class RequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    explicit RequestHandlerFactory(std::shared_ptr<services::InventoryService> inventoryService)
        : inventoryService_(inventoryService) {}
    
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) override {
        utils::Logger::info("Incoming request: {} {}", request.getMethod(), request.getURI());
        
        // TODO: Implement proper routing
        // For now, route all requests to InventoryController
        return new controllers::InventoryController(inventoryService_);
    }
    
private:
    std::shared_ptr<services::InventoryService> inventoryService_;
};

Server::Server(int port) : port_(port) {}

Server::~Server() {
    stop();
}

void Server::setInventoryService(std::shared_ptr<services::InventoryService> service) {
    inventoryService_ = service;
}

void Server::start() {
    Poco::Net::ServerSocket socket(port_);
    Poco::Net::HTTPServerParams* params = new Poco::Net::HTTPServerParams;
    params->setMaxQueued(100);
    params->setMaxThreads(16);
    
    httpServer_ = std::make_unique<Poco::Net::HTTPServer>(
        new RequestHandlerFactory(inventoryService_),
        socket,
        params
    );
    
    httpServer_->start();
    utils::Logger::info("HTTP Server started on port {}", port_);
    
    // Wait for termination signal
    utils::Logger::info("Press Ctrl+C to stop the server");
    waitForTerminationRequest();
    
    stop();
}

void Server::stop() {
    if (httpServer_) {
        utils::Logger::info("Stopping HTTP server...");
        httpServer_->stop();
        httpServer_.reset();
    }
}

} // namespace inventory

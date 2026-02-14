#include "inventory/Server.hpp"
#include "inventory/controllers/InventoryController.hpp"
#include "inventory/controllers/HealthController.hpp"
#include "inventory/controllers/SwaggerController.hpp"
#include "inventory/controllers/ClaimsController.hpp"
#include "inventory/utils/Logger.hpp"
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/URI.h>
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

class RequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    explicit RequestHandlerFactory(std::shared_ptr<services::InventoryService> inventoryService)
        : inventoryService_(inventoryService) {}
    
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) override {
        utils::Logger::info("Incoming request: {} {}", request.getMethod(), request.getURI());

        Poco::URI uri(request.getURI());
        const std::string path = uri.getPath();

        RouteTarget target = resolveRoute(path);
        switch (target) {
            case RouteTarget::Health:
                return new controllers::HealthController();
            case RouteTarget::Swagger:
                return new controllers::SwaggerController();
            case RouteTarget::Claims:
                return new controllers::ClaimsController();
            case RouteTarget::Inventory:
            default:
                // TODO: Implement more complete routing
                // Default: route to InventoryController
                return new controllers::InventoryController(inventoryService_);
        }
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

#pragma once

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <memory>

namespace warehouse::services {
    class WarehouseService;
    class LocationService;
}

namespace warehouse {

/**
 * @brief HTTP Server wrapper
 */
class Server {
public:
    struct Config {
        std::string host = "0.0.0.0";
        int port = 8080;
        int maxThreads = 10;
        int maxQueued = 100;
    };
    
    Server(const Config& config,
           std::shared_ptr<services::WarehouseService> warehouseService,
           std::shared_ptr<services::LocationService> locationService);
    
    ~Server();
    
    void start();
    void stop();
    bool isRunning() const;

private:
    Config config_;
    std::unique_ptr<Poco::Net::HTTPServer> httpServer_;
    std::shared_ptr<services::WarehouseService> warehouseService_;
    std::shared_ptr<services::LocationService> locationService_;
    bool running_ = false;
    
    class RequestHandlerFactory;
};

} // namespace warehouse

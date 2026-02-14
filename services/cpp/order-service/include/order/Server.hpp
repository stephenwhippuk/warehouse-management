#pragma once

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <memory>

namespace order::services {
    class OrderService;
}

namespace order {

struct Config {
    std::string host = "0.0.0.0";
    int port = 8082;
    int maxThreads = 10;
    int maxQueued = 100;
};

class Server {
public:
    Server(const Config& config, std::shared_ptr<services::OrderService> orderService);
    ~Server();
    
    void start();
    void stop();
    bool isRunning() const;

private:
    class RequestHandlerFactory;
    
    Config config_;
    std::shared_ptr<services::OrderService> orderService_;
    std::unique_ptr<Poco::Net::HTTPServer> httpServer_;
    bool running_ = false;
};

} // namespace order

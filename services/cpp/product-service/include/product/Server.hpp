#pragma once

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <memory>
#include <string>

namespace product {
    namespace services { class ProductService; }

/**
 * @brief HTTP Server wrapper and request router
 */
class Server {
public:
    Server(int port, const std::string& host = "0.0.0.0");
    ~Server();
    
    void setService(std::shared_ptr<services::ProductService> service);
    void start();
    void stop();
    bool isRunning() const { return isRunning_; }

private:
    int port_;
    std::string host_;
    bool isRunning_ = false;
    std::shared_ptr<services::ProductService> service_;
    std::unique_ptr<Poco::Net::HTTPServer> httpServer_;
};

}  // namespace product

#pragma once

#include <http-framework/HttpHost.hpp>
#include <http-framework/IServiceProvider.hpp>
#include <memory>
#include <string>

namespace inventory {

class Server {
public:
    explicit Server(int port);
    ~Server();
    
    void setServiceProvider(std::shared_ptr<http::IServiceProvider> provider);
    void start();
    void stop();
    
private:
    int port_;
    std::unique_ptr<http::HttpHost> httpHost_;
    std::shared_ptr<http::IServiceProvider> serviceProvider_;
};

} // namespace inventory

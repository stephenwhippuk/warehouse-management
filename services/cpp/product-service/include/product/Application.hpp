#pragma once

#include "http-framework/IServiceProvider.hpp"
#include "http-framework/HttpHost.hpp"
#include <memory>
#include <string>

namespace product {

/**
 * @brief Main application class
 * 
 * Manages service initialization, DI container, and HTTP server using http-framework
 */
class Application {
public:
    Application();
    
    int run(int argc, char* argv[]);

private:
    std::shared_ptr<http::IServiceProvider> serviceProvider_;
    std::unique_ptr<http::HttpHost> httpHost_;
    
    void initialize();
    void start();
    void stop();
};

}  // namespace product

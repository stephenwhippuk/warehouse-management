#pragma once

#include "http-framework/IServiceProvider.hpp"
#include "http-framework/HttpHost.hpp"
#include <contract-plugin/ContractPlugin.hpp>
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
    std::shared_ptr<contract::ContractPlugin> contractPlugin_;  // Must be member to keep alive!
    
    void initialize();
    void start();
    void stop();
};

}  // namespace product

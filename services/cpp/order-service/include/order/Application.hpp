#pragma once

#include <http-framework/HttpHost.hpp>
#include <http-framework/IServiceProvider.hpp>
#include <memory>
#include <string>

namespace order {

/**
 * @brief Main application class with DI container and HTTP host
 */
class Application {
public:
    Application();
    ~Application();
    
    bool initialize();
    void run();
    void shutdown();
    
private:
    void initializeLogger();
    void initializeConfig();
    void initializeDI();
    void initializeHttpServer();
    void initializeMessaging();
    
    std::string dbConnectionString_;
    int serverPort_ = 8083;
    std::string serverHost_ = "0.0.0.0";
    
    std::shared_ptr<http::IServiceProvider> serviceProvider_;
    std::unique_ptr<http::HttpHost> httpHost_;
};

} // namespace order

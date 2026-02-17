#pragma once

#include "utils/Config.hpp"
#include "utils/Database.hpp"
#include "utils/Logger.hpp"
#include <http-framework/HttpHost.hpp>
#include <http-framework/IServiceProvider.hpp>
#include <contract-plugin/ContractPlugin.hpp>
#include <memory>
#include <string>

namespace warehouse {

/**
 * @brief Main application class
 */
class Application {
public:
    Application();
    ~Application() = default;
    
    int run(int argc, char* argv[]);
    void initialize();
    void start();
    void stop();

private:
    std::unique_ptr<http::HttpHost> httpHost_;
    std::shared_ptr<http::IServiceProvider> serviceProvider_;
    std::shared_ptr<contract::ContractPlugin> contractPlugin_;  // Must be member to keep alive!
};

}  // namespace warehouse

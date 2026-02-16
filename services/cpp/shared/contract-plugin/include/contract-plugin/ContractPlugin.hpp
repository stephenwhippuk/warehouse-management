#pragma once

#include <http-framework/IPlugin.hpp>
#include <http-framework/NamespacedServiceCollection.hpp>
#include <http-framework/ControllerBase.hpp>
#include <http-framework/Middleware.hpp>
#include <http-framework/IServiceProvider.hpp>
#include "ContractConfig.hpp"
#include <memory>
#include <vector>

namespace contract {

/**
 * @brief Plugin that provides contract validation, claims, and swagger endpoints
 * 
 * This plugin:
 * - Registers ContractValidationMiddleware for response validation
 * - Registers ClaimsController (/api/claims endpoint)
 * - Registers SwaggerController (/api/swagger.json endpoint)
 * 
 * Configuration via environment variables or JSON:
 * - CONTRACT_CLAIMS_PATH
 * - CONTRACT_CONTRACTS_PATH
 * - CONTRACT_GLOBAL_CONTRACTS_PATH
 * - CONTRACT_ENABLE_VALIDATION
 * - CONTRACT_STRICT_MODE
 * - CONTRACT_ENABLE_SWAGGER
 * - CONTRACT_ENABLE_CLAIMS
 * 
 * Usage:
 * 
 * In Application.cpp:
 * 
 *     // Before building service provider
 *     http::PluginManager pluginManager(services);
 *     pluginManager.loadPlugin("./libcontract-plugin.so");
 *     
 *     auto provider = services.buildServiceProvider();
 *     
 *     // The plugin will have registered middleware and controllers
 *     // Get middleware from DI container:
 *     auto validationMiddleware = provider->getService<contract::ContractValidationMiddleware>();
 *     httpHost_->use(validationMiddleware);
 *     
 *     // Controllers are returned via getControllers() and can be registered:
 *     for (auto& controller : plugin->getControllers()) {
 *         httpHost_->addController(controller);
 *     }
 */
class ContractPlugin : public http::IPlugin {
public:
    /**
     * @brief Create plugin with default configuration from environment
     */
    ContractPlugin();
    
    /**
     * @brief Create plugin with custom configuration
     * @param config Configuration for the plugin
     */
    explicit ContractPlugin(const ContractConfig& config);
    
    /**
     * @brief Get plugin metadata
     */
    http::PluginInfo getInfo() const override;
    
    /**
     * @brief Register plugin services (middleware, etc.)
     * @param services Service collection to register into
     */
    void registerServices(http::NamespacedServiceCollection& services) override;
    
    /**
     * @brief Configure plugin after creation (for per-service customization)
     * @param config Custom configuration for this service instance
     * 
     * Call this BEFORE calling getControllers() or registering services.
     * Allows each service to specify its own contract paths:
     * 
     * auto plugin = std::make_unique<contract::ContractPlugin>();
     * 
     * contract::ContractConfig config;
     * config.claimsPath = "./claims.json";
     * config.contractsPath = "./contracts";
     * config.swaggerTitle = "Inventory Service API";
     * 
     * plugin->configure(config);
     */
    void configure(const ContractConfig& config);
    
    /**
     * @brief Get plugin controllers
     * @return Vector of controllers (ClaimsController, SwaggerController)
     */
    std::vector<std::shared_ptr<http::ControllerBase>> getControllers() override;

    /**
     * @brief Get plugin middleware (optional)
     */
    std::vector<std::shared_ptr<http::Middleware>> getMiddleware(
        http::IServiceProvider& provider) override;
    
    /**
     * @brief Cleanup on shutdown
     */
    void onShutdown() override;

private:
    ContractConfig config_;
    std::vector<std::shared_ptr<http::ControllerBase>> controllers_;
};

} // namespace contract

/**
 * @brief Plugin factory function (required for dynamic loading)
 */
extern "C" {
    std::unique_ptr<http::IPlugin> createPlugin();
}

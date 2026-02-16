#include "contract-plugin/ContractPlugin.hpp"
#include "contract-plugin/ContractValidationMiddleware.hpp"
#include "contract-plugin/ClaimsController.hpp"
#include "contract-plugin/ClaimsService.hpp"
#include "contract-plugin/ClaimsLoader.hpp"
#include "contract-plugin/SwaggerController.hpp"
#include "contract-plugin/SwaggerService.hpp"
#include <http-framework/ServiceNamespace.hpp>
#include <spdlog/spdlog.h>

namespace contract {

ContractPlugin::ContractPlugin()
    : config_(ContractConfig::fromEnvironment()) {
    spdlog::info("ContractPlugin created with default configuration");
}

ContractPlugin::ContractPlugin(const ContractConfig& config)
    : config_(config) {
    spdlog::info("ContractPlugin created with custom configuration");
}

void ContractPlugin::configure(const ContractConfig& config) {
    spdlog::info("ContractPlugin reconfigured");
    config_ = config;
    
    // Clear existing controllers so they'll be recreated with new config
    controllers_.clear();
}

http::PluginInfo ContractPlugin::getInfo() const {
    return {
        "contract-plugin",
        "1.0.0",
        "Contract validation, claims, and swagger endpoints",
        "Warehouse Management System"
    };
}

void ContractPlugin::registerServices(http::NamespacedServiceCollection& services) {
    spdlog::info("ContractPlugin registering services");
    
    // Register ContractValidationMiddleware as a singleton
    // Services can retrieve it via DI and add to middleware pipeline
    services.addService<ContractValidationMiddleware>(
        [config = this->config_](http::IServiceProvider&) {
            return std::make_shared<ContractValidationMiddleware>(config);
        },
        http::ServiceLifetime::Singleton
    );
    
    // Register ClaimsLoader as singleton (shared filesystem loader)
    services.addService<IClaimsLoader>(
        [](http::IServiceProvider&) {
            return std::make_shared<ClaimsLoader>();
        },
        http::ServiceLifetime::Singleton
    );
    
    // Register ClaimsService as transient
    // Each service gets its own instance with injected loader
    services.addService<IClaimsService>(
        [config = this->config_](http::IServiceProvider& provider) {
            auto loader = provider.getService<IClaimsLoader>();
            return std::make_shared<ClaimsService>(config, loader);
        },
        http::ServiceLifetime::Transient
    );
    
    // Register SwaggerService as transient
    // Uses ClaimsLoader for loading claims.json
    services.addService<ISwaggerService>(
        [config = this->config_](http::IServiceProvider& provider) {
            auto loader = provider.getService<IClaimsLoader>();
            return std::make_shared<SwaggerService>(config, loader);
        },
        http::ServiceLifetime::Transient
    );
    
    spdlog::info("ContractPlugin registered ContractValidationMiddleware, ClaimsLoader, ClaimsService, and SwaggerService");
}

std::vector<std::shared_ptr<http::ControllerBase>> ContractPlugin::getControllers() {
    if (!controllers_.empty()) {
        return controllers_; // Already created
    }
    
    spdlog::info("ContractPlugin creating controllers");
    
    // Create controllers based on configuration
    if (config_.enableClaims) {
        // Create ClaimsLoader and ClaimsService instances for the controller
        auto loader = std::make_shared<ClaimsLoader>();
        auto claimsService = std::make_shared<ClaimsService>(config_, loader);
        controllers_.push_back(std::make_shared<ClaimsController>(config_, claimsService));
        spdlog::info("ContractPlugin registered ClaimsController with 5 endpoints");
    }
    
    if (config_.enableSwagger) {
        // Create SwaggerService with loader and pass to controller
        auto loader = std::make_shared<ClaimsLoader>();
        auto swaggerService = std::make_shared<SwaggerService>(config_, loader);
        controllers_.push_back(std::make_shared<SwaggerController>(config_, swaggerService));
        spdlog::info("ContractPlugin registered SwaggerController with injected SwaggerService");
    }
    
    return controllers_;
}

std::vector<std::shared_ptr<http::Middleware>> ContractPlugin::getMiddleware(
    http::IServiceProvider& provider) {
    std::vector<std::shared_ptr<http::Middleware>> middleware;

    if (!config_.enableValidation) {
        return middleware;
    }

    std::string pluginNamespace = http::ServiceNamespace::pluginNamespace(getInfo().name);
    auto validation = provider.getService<ContractValidationMiddleware>(pluginNamespace);
    middleware.push_back(validation);
    return middleware;
}

void ContractPlugin::onShutdown() {
    spdlog::info("ContractPlugin shutting down");
    controllers_.clear();
}

} // namespace contract

// Plugin factory function (C linkage for dynamic loading)
extern "C" {
    std::unique_ptr<http::IPlugin> createPlugin() {
        return std::make_unique<contract::ContractPlugin>();
    }
}


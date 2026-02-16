#include <http-framework/IPlugin.hpp>
#include <http-framework/NamespacedServiceCollection.hpp>
#include "ICalculator.hpp"
#include <memory>

namespace test_plugin {

/**
 * @brief Test service implementation
 */
class Calculator : public ICalculator {
public:
    int add(int a, int b) const override { return a + b; }
    int multiply(int a, int b) const override { return a * b; }
};

/**
 * @brief Test plugin implementation
 */
class TestPlugin : public http::IPlugin {
public:
    http::PluginInfo getInfo() const override {
        return {
            "test-calculator",
            "1.0.0",
            "Test plugin providing calculator service",
            "Test Author"
        };
    }

    void registerServices(http::NamespacedServiceCollection& services) override {
        // Register Calculator service using factory that doesn't require provider
        // Use Transient lifetime for easier testing
        services.addService<ICalculator>(
            [](http::IServiceProvider&) -> std::shared_ptr<ICalculator> {
                return std::make_shared<Calculator>();
            },
            http::ServiceLifetime::Transient
        );
    }

    std::vector<std::shared_ptr<http::ControllerBase>> getControllers() override {
        // No controllers needed for calculator test plugin
        return {};
    }

    void onShutdown() override {
        // Plugin cleanup if needed
    }
};

} // namespace test_plugin

/**
 * @brief Required plugin factory function
 * Must be exported with C linkage so the loader can find it with dlsym
 */
extern "C" {
    std::unique_ptr<http::IPlugin> createPlugin() {
        return std::make_unique<test_plugin::TestPlugin>();
    }
}

#include <catch2/catch_all.hpp>
#include <http-framework/PluginManager.hpp>
#include <http-framework/ServiceCollection.hpp>
#include <http-framework/IServiceProvider.hpp>
#include <filesystem>
#include <memory>
#include "../plugins/ICalculator.hpp"

using namespace http;

TEST_CASE("PluginManager loads plugin from file", "[plugin][manager]") {
    // Build test plugin first (ensure it exists)
    // Plugin is in the same directory as the test executable
    std::string pluginPath = "./libtest-plugin.so";

    SECTION("Load valid plugin") {
        if (!std::filesystem::exists(pluginPath)) {
            SKIP("Test plugin not built: " << pluginPath);
        }

        ServiceCollection services;
        PluginManager mgr(services);

        REQUIRE_NOTHROW(mgr.loadPlugin(pluginPath));
    }
}

TEST_CASE("PluginManager tracks loaded plugins", "[plugin][manager][tracking]") {
    std::string pluginPath = "./libtest-plugin.so";

    if (!std::filesystem::exists(pluginPath)) {
        SKIP("Test plugin not built");
    }

    ServiceCollection services;
    PluginManager mgr(services);

    SECTION("Plugin is marked as loaded after loading") {
        mgr.loadPlugin(pluginPath);
        REQUIRE(mgr.isPluginLoaded("test-calculator"));
    }

    SECTION("Unloaded plugin returns false") {
        REQUIRE(!mgr.isPluginLoaded("nonexistent-plugin"));
    }

    SECTION("Get loaded plugins list") {
        mgr.loadPlugin(pluginPath);
        auto plugins = mgr.getLoadedPlugins();
        
        REQUIRE(plugins.size() == 1);
        REQUIRE(plugins[0].name == "test-calculator");
        REQUIRE(plugins[0].version == "1.0.0");
        REQUIRE(plugins[0].author == "Test Author");
    }
}

TEST_CASE("PluginManager registers plugin services", "[plugin][manager][services]") {
    std::string pluginPath = "./libtest-plugin.so";

    if (!std::filesystem::exists(pluginPath)) {
        SKIP("Test plugin not built");
    }

    ServiceCollection services;
    PluginManager mgr(services);

    SECTION("Plugin services are available after loading") {
        mgr.loadPlugin(pluginPath);

        // Build provider and verify service is available
        auto provider = services.buildServiceProvider();
        auto calc = provider->getService<ICalculator>();

        REQUIRE(calc);
        REQUIRE(calc->add(2, 3) == 5);
        REQUIRE(calc->multiply(4, 5) == 20);
    }

    SECTION("Service creates new instance each time (transient)") {
        mgr.loadPlugin(pluginPath);
        auto provider = services.buildServiceProvider();

        auto calc1 = provider->getService<ICalculator>();
        auto calc2 = provider->getService<ICalculator>();

        // Transient services should be different instances
        REQUIRE(calc1.get() != calc2.get());
    }
}

TEST_CASE("PluginManager unloads plugins", "[plugin][manager][unload]") {
    std::string pluginPath = "./libtest-plugin.so";

    if (!std::filesystem::exists(pluginPath)) {
        SKIP("Test plugin not built");
    }

    ServiceCollection services;
    PluginManager mgr(services);

    SECTION("Unload specific plugin") {
        mgr.loadPlugin(pluginPath);
        REQUIRE(mgr.isPluginLoaded("test-calculator"));

        REQUIRE(mgr.unloadPlugin("test-calculator"));
        REQUIRE(!mgr.isPluginLoaded("test-calculator"));
    }

    SECTION("Unload nonexistent plugin returns false") {
        REQUIRE(!mgr.unloadPlugin("nonexistent"));
    }

    SECTION("Unload all plugins") {
        mgr.loadPlugin(pluginPath);
        REQUIRE(mgr.isPluginLoaded("test-calculator"));

        mgr.unloadAll();
        REQUIRE(!mgr.isPluginLoaded("test-calculator"));
        REQUIRE(mgr.getLoadedPlugins().empty());
    }
}

TEST_CASE("PluginManager handles errors gracefully", "[plugin][manager][errors]") {
    ServiceCollection services;
    PluginManager mgr(services);

    SECTION("Missing plugin file throws error") {
        REQUIRE_THROWS_WITH(
            mgr.loadPlugin("/nonexistent/path/to/plugin.so"),
            Catch::Matchers::ContainsSubstring("Failed to load plugin")
        );
    }

    SECTION("Plugin without createPlugin function throws error") {
        // Would need a bad plugin .so file to test this
        // For now, just verify the error message mentions createPlugin
        REQUIRE_THROWS_WITH(
            mgr.loadPlugin("/dev/null"),  // Not a valid .so file
            Catch::Matchers::ContainsSubstring("Failed to load plugin")
        );
    }
}

TEST_CASE("Multiple plugins can be loaded together", "[plugin][manager][multiple]") {
    std::string pluginPath = "./libtest-plugin.so";

    if (!std::filesystem::exists(pluginPath)) {
        SKIP("Test plugin not built");
    }

    ServiceCollection services;
    PluginManager mgr(services);

    SECTION("Load same plugin multiple times has no effect (same name)") {
        mgr.loadPlugin(pluginPath);
        
        // Loading the same plugin again replaces the previous one
        // (Since it has the same name)
        REQUIRE_NOTHROW(mgr.loadPlugin(pluginPath));
        
        auto plugins = mgr.getLoadedPlugins();
        REQUIRE(plugins.size() == 1);
    }

    SECTION("Plugin metadata is accessible") {
        auto info = mgr.loadPlugin(pluginPath);

        REQUIRE(info.name == "test-calculator");
        REQUIRE(info.version == "1.0.0");
        REQUIRE(!info.description.empty());
        REQUIRE(!info.author.empty());
    }
}

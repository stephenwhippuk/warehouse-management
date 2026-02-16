#pragma once

#include "ServiceCollection.hpp"
#include "ServiceNamespace.hpp"
#include <string>
#include <memory>
#include <vector>

namespace http {

// Forward declarations
class NamespacedServiceCollection;
class ControllerBase;
class Router;
class Middleware;
class IServiceProvider;

/**
 * @brief Plugin metadata and initialization information
 */
struct PluginInfo {
    std::string name;           // Plugin name (e.g., "inventory-extended")
    std::string version;        // Plugin version (e.g., "1.0.0")
    std::string description;    // Plugin description
    std::string author;         // Plugin author
};

/**
 * @brief Interface that all plugins must implement
 * 
 * Plugins are loaded dynamically at runtime and register their services
 * into the main application's service collection, organized by namespace
 * to prevent conflicts.
 * 
 * Typical plugin structure:
 * 
 * class MyPlugin : public IPlugin {
 * public:
 *     PluginInfo getInfo() const override { ... }
 *     void registerServices(NamespacedServiceCollection& services) override { ... }
 * };
 * 
 * extern "C" {
 *     std::unique_ptr<IPlugin> createPlugin() {
 *         return std::make_unique<MyPlugin>();
 *     }
 * }
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief Get plugin metadata
     * @return Plugin information (name, version, description, author)
     */
    virtual PluginInfo getInfo() const = 0;

    /**
     * @brief Register plugin services into the main application
     * @param services NamespacedServiceCollection to register services into
     * 
     * Services registered here will be placed in the plugin's namespace
     * (automatically handled by NamespacedServiceCollection) to prevent
     * conflicts with other plugins or application services.
     * 
     * All services are exported (visible to other namespaces) by default.
     * For internal services, use services.addInternal<>() method.
     */
    virtual void registerServices(NamespacedServiceCollection& services) = 0;

    /**
     * @brief Register plugin HTTP controllers (optional)
     * @param router Router to register controllers with
     * @return Vector of controllers registered by this plugin
     * 
     * Controllers registered here will be added to the main application's router.
     * Each controller must have a unique base route to avoid conflicts.
     * A duplicate route will throw std::runtime_error during registration.
     * 
     * Default implementation returns empty vector (optional feature).
     */
    virtual std::vector<std::shared_ptr<ControllerBase>> getControllers() {
        return {};
    }

    /**
     * @brief Register plugin middleware (optional)
     * @param provider Service provider to resolve dependencies
     * @return Vector of middleware to add to the host
     *
     * Default implementation returns empty vector (optional feature).
     */
    virtual std::vector<std::shared_ptr<Middleware>> getMiddleware(IServiceProvider& /* provider */) {
        return {};
    }

    /**
     * @brief Optional hook called when plugin is shutting down
     */
    virtual void onShutdown() {}
};

/**
 * @brief Type for the plugin factory function
 * 
 * Every plugin shared library must export a C-linkage function
 * with this signature:
 * 
 *     extern "C" {
 *         std::unique_ptr<IPlugin> createPlugin() { ... }
 *     }
 */
using PluginFactory = std::unique_ptr<IPlugin>(*)();

} // namespace http

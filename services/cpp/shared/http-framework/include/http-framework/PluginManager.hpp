#pragma once

#include "IPlugin.hpp"
#include "ServiceCollection.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace http {

/**
 * @brief Manages dynamic loading of plugins and service registration
 * 
 * PluginManager handles:
 * - Loading shared libraries (.so files) at runtime
 * - Creating plugin instances via factory functions
 * - Registering plugin services into the main ServiceCollection
 * - Tracking loaded plugins
 * - Cleanup on destruction
 * 
 * Usage:
 * 
 *     ServiceCollection services;
 *     PluginManager pluginMgr(services);
 *     pluginMgr.loadPlugin("/path/to/libmyplugin.so");
 *     pluginMgr.loadPlugin("/path/to/libotherplugin.so");
 *     // Services from plugins are now available
 */
class PluginManager {
public:
    /**
     * @brief Create a PluginManager
     * @param services ServiceCollection where plugins will register services
     */
    explicit PluginManager(ServiceCollection& services);

    /**
     * @brief Destructor - unloads all plugins
     */
    ~PluginManager();

    // Non-copyable
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    /**
     * @brief Load a plugin from a shared library file
     * @param filePath Path to .so file (e.g., "/path/to/libmyplugin.so")
     * @throws std::runtime_error if plugin cannot be loaded
     * @return Plugin info (metadata from the loaded plugin)
     * 
     * The .so file must export a C-linkage function:
     *     extern "C" std::unique_ptr<IPlugin> createPlugin()
     */
    PluginInfo loadPlugin(const std::string& filePath);

    /**
     * @brief Check if a plugin is loaded by name
     * @param pluginName Name of the plugin
     * @return true if plugin is loaded, false otherwise
     */
    bool isPluginLoaded(const std::string& pluginName) const;

    /**
     * @brief Get list of all loaded plugins
     * @return Vector of PluginInfo for all loaded plugins
     */
    std::vector<PluginInfo> getLoadedPlugins() const;

    /**
     * @brief Unload a specific plugin
     * @param pluginName Name of the plugin to unload
     * @return true if plugin was unloaded, false if not found
     */
    bool unloadPlugin(const std::string& pluginName);

    /**
     * @brief Unload all plugins
     */
    void unloadAll();

private:
    /**
     * @brief Plugin entry - holds plugin instance and library handle
     */
    struct PluginEntry {
        std::unique_ptr<IPlugin> plugin;
        void* handle;  // dlopen handle
    };

    ServiceCollection& services_;
    std::map<std::string, PluginEntry> loadedPlugins_;

    /**
     * @brief Extract plugin name from file path
     * @param filePath Path like "/path/to/libmyplugin.so"
     * @return Plugin name like "myplugin"
     */
    static std::string extractPluginName(const std::string& filePath);
};

} // namespace http

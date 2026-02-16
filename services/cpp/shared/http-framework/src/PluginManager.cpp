#include "http-framework/PluginManager.hpp"
#include "http-framework/ServiceNamespace.hpp"
#include "http-framework/NamespacedServiceCollection.hpp"
#include "http-framework/ControllerBase.hpp"
#include <dlfcn.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>

namespace http {

PluginManager::PluginManager(ServiceCollection& services)
    : services_(services) {
}

PluginManager::~PluginManager() {
    unloadAll();
}

PluginInfo PluginManager::loadPlugin(const std::string& filePath) {
    // Load the shared library
    void* handle = dlopen(filePath.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        std::string error = dlerror();
        spdlog::error("Failed to load plugin from {}: {}", filePath, error);
        throw std::runtime_error("Failed to load plugin: " + error);
    }

    // Clear any previous errors
    dlerror();

    // Get the plugin factory function
    PluginFactory factory = reinterpret_cast<PluginFactory>(dlsym(handle, "createPlugin"));
    const char* dlError = dlerror();
    if (dlError) {
        std::string error = dlError;
        dlclose(handle);
        spdlog::error("Plugin missing createPlugin function in {}: {}", filePath, error);
        throw std::runtime_error("Plugin missing createPlugin function: " + error);
    }

    // Create the plugin instance
    std::unique_ptr<IPlugin> plugin;
    try {
        plugin = factory();
    } catch (const std::exception& e) {
        dlclose(handle);
        spdlog::error("Failed to create plugin instance from {}: {}", filePath, e.what());
        throw std::runtime_error("Failed to create plugin instance: " + std::string(e.what()));
    }

    if (!plugin) {
        dlclose(handle);
        spdlog::error("Plugin factory returned null from {}", filePath);
        throw std::runtime_error("Plugin factory returned null");
    }

    // Get plugin metadata
    PluginInfo info = plugin->getInfo();
    spdlog::info("Loading plugin '{}' v{} from {}", info.name, info.version, filePath);

    // Create plugin namespace automatically
    std::string pluginNamespace = ServiceNamespace::pluginNamespace(info.name);
    NamespacedServiceCollection nsCollection(services_, pluginNamespace);

    // Register plugin services in plugin's namespace
    try {
        // Pass NamespacedServiceCollection directly - all registrations go to plugin namespace
        plugin->registerServices(nsCollection);
        spdlog::info("Plugin '{}' registered service(s) in namespace '{}'",
                     info.name,
                     pluginNamespace);
    } catch (const std::exception& e) {
        dlclose(handle);
        spdlog::error("Plugin '{}' failed to register services: {}", info.name, e.what());
        throw std::runtime_error("Plugin failed to register services: " + std::string(e.what()));
    }

    // Register plugin controllers (optional)
    try {
        auto controllers = plugin->getControllers();
        if (!controllers.empty()) {
            spdlog::info("Plugin '{}' registering {} controller(s)", info.name, controllers.size());
            for (auto& controller : controllers) {
                if (!controller) {
                    throw std::runtime_error("Plugin returned null controller");
                }
                // Controller registration will throw on duplicate routes
                // HttpHost will catch and provide error context
                spdlog::debug("Plugin '{}' registering controller at: {}", 
                             info.name, controller->getBaseRoute());
            }
        }
    } catch (const std::exception& e) {
        dlclose(handle);
        spdlog::error("Plugin '{}' failed to register controllers: {}", info.name, e.what());
        throw std::runtime_error("Plugin controller registration failed: " + std::string(e.what()));
    }

    // Store the plugin
    loadedPlugins_[info.name] = {std::move(plugin), handle};
    spdlog::info("Plugin '{}' loaded successfully", info.name);

    return info;
}

bool PluginManager::isPluginLoaded(const std::string& pluginName) const {
    return loadedPlugins_.find(pluginName) != loadedPlugins_.end();
}

std::vector<PluginInfo> PluginManager::getLoadedPlugins() const {
    std::vector<PluginInfo> result;
    for (const auto& [name, entry] : loadedPlugins_) {
        result.push_back(entry.plugin->getInfo());
    }
    return result;
}

bool PluginManager::unloadPlugin(const std::string& pluginName) {
    auto it = loadedPlugins_.find(pluginName);
    if (it == loadedPlugins_.end()) {
        return false;
    }

    auto& entry = it->second;
    try {
        entry.plugin->onShutdown();
    } catch (const std::exception& e) {
        spdlog::warn("Plugin '{}' shutdown hook failed: {}", pluginName, e.what());
    }

    dlclose(entry.handle);
    loadedPlugins_.erase(it);
    spdlog::info("Plugin '{}' unloaded", pluginName);

    return true;
}

void PluginManager::unloadAll() {
    // Unload in reverse order of loading
    while (!loadedPlugins_.empty()) {
        auto it = loadedPlugins_.begin();
        std::string pluginName = it->first;
        unloadPlugin(pluginName);
    }
}

std::string PluginManager::extractPluginName(const std::string& filePath) {
    // Extract "myplugin" from "/path/to/libmyplugin.so"
    size_t lastSlash = filePath.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos) 
        ? filePath.substr(lastSlash + 1) 
        : filePath;

    // Remove .so extension
    size_t soPos = filename.find(".so");
    if (soPos != std::string::npos) {
        filename = filename.substr(0, soPos);
    }

    // Remove "lib" prefix if present
    if (filename.substr(0, 3) == "lib") {
        filename = filename.substr(3);
    }

    return filename;
}

} // namespace http

#pragma once

#include <string>
#include <stdexcept>

namespace http {

/**
 * @brief Service namespace constants and utilities
 * 
 * Namespaces organize services by plugin/scope to prevent naming conflicts.
 * Each namespace is isolated - you can have ILogger in "core" and a different
 * ILogger in "plugin-auth" without conflict.
 */
class ServiceNamespace {
public:
    /**
     * @brief Global namespace - for core application services
     * All services without explicit namespace go here
     */
    static constexpr std::string_view global() { return "global"; }

    /**
     * @brief Create a plugin namespace from plugin name
     * @param pluginName Name of the plugin (e.g., "auth-plugin")
     * @return Namespace for that plugin (e.g., "plugin:auth-plugin")
     */
    static std::string pluginNamespace(const std::string& pluginName) {
        return "plugin:" + pluginName;
    }

    /**
     * @brief Check if a namespace is a plugin namespace
     * @param ns Namespace to check
     * @return true if namespace is for a plugin
     */
    static bool isPluginNamespace(const std::string& ns) {
        return ns.substr(0, 7) == "plugin:";
    }

    /**
     * @brief Extract plugin name from plugin namespace
     * @param ns Plugin namespace (e.g., "plugin:auth-plugin")
     * @return Plugin name (e.g., "auth-plugin")
     */
    static std::string extractPluginName(const std::string& ns) {
        if (!isPluginNamespace(ns)) {
            throw std::invalid_argument("Not a plugin namespace: " + ns);
        }
        return ns.substr(7);  // Skip "plugin:" prefix
    }

    /**
     * @brief Validate namespace name
     * @param ns Namespace to validate
     * @throws std::invalid_argument if namespace is invalid
     */
    static void validate(const std::string& ns) {
        if (ns.empty()) {
            throw std::invalid_argument("Namespace cannot be empty");
        }
        
        // Check for invalid characters
        for (char c : ns) {
            if (!std::isalnum(c) && c != ':' && c != '-' && c != '_') {
                throw std::invalid_argument("Invalid namespace format: " + ns);
            }
        }
        
        // If contains colon, must be plugin: namespace
        if (ns.find(':') != std::string::npos) {
            if (!isPluginNamespace(ns)) {
                throw std::invalid_argument("Invalid namespace format: " + ns);
            }
            std::string pluginName = extractPluginName(ns);
            if (pluginName.empty()) {
                throw std::invalid_argument("Plugin name cannot be empty");
            }
        }
    }
};

/**
 * @brief Registration visibility - controls where a service can be resolved
 */
enum class ServiceVisibility {
    /**
     * @brief Service is internal to namespace - only resolvable within same namespace
     * Other namespaces cannot see this service
     */
    Internal,

    /**
     * @brief Service is exported - resolvable across namespace boundaries
     * Plugin can override or extend services from other namespaces
     */
    Exported,
};

} // namespace http

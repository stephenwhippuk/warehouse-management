#pragma once

namespace http {

/**
 * @brief Defines the lifetime of a service in the DI container
 */
enum class ServiceLifetime {
    /**
     * @brief New instance created for every request
     * Use for lightweight, stateless services
     */
    Transient,
    
    /**
     * @brief One instance per scope (typically per HTTP request)
     * DEFAULT - Most services should be scoped
     */
    Scoped,
    
    /**
     * @brief Single instance for the entire application lifetime
     * Use for expensive resources (database pools, caches, etc.)
     * Thread-safe creation guaranteed via std::call_once
     */
    Singleton
};

} // namespace http

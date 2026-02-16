#pragma once

#include <memory>

namespace http {

// Forward declaration
class IServiceProvider;

/**
 * @brief Represents a scope for scoped services (typically per HTTP request)
 * 
 * Scoped services created within this scope will be reused for the duration
 * of the scope and destroyed when the scope ends (RAII).
 */
class IServiceScope {
public:
    virtual ~IServiceScope() = default;
    
    /**
     * @brief Get the service provider for this scope
     * 
     * Services resolved from this provider will be scoped to this scope's lifetime.
     */
    virtual IServiceProvider& getServiceProvider() = 0;
};

} // namespace http

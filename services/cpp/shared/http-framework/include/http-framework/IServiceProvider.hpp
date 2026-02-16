#pragma once

#include <memory>
#include <typeindex>
#include <string>

namespace http {

// Forward declarations
class IServiceScope;
class ServiceNamespace;

/**
 * @brief Service provider interface for dependency injection
 * 
 * Resolves services registered in the ServiceCollection.
 * Services receive IServiceProvider& to access their dependencies.
 * 
 * Services can be namespaced to prevent conflicts across plugins.
 */
class IServiceProvider {
public:
    virtual ~IServiceProvider() = default;
    
    /**
     * @brief Get a required service by type from global namespace
     * @throws std::runtime_error if service not found
     */
    template<typename T>
    std::shared_ptr<T> getService() {
        return getService<T>("global");  // Global namespace
    }
    
    /**
     * @brief Get a required service by type from specific namespace
     * @param ns Namespace (e.g., "global", "plugin:auth", etc.)
     * @throws std::runtime_error if service not found
     */
    template<typename T>
    std::shared_ptr<T> getService(const std::string& ns) {
        auto service = getServiceInternal(std::type_index(typeid(T)), ns);
        if (!service) {
            throw std::runtime_error(
                "Service not found: " + std::string(typeid(T).name()) + 
                " in namespace: " + ns
            );
        }
        return std::static_pointer_cast<T>(service);
    }
    
    /**
     * @brief Get an optional service from global namespace (returns nullptr if not found)
     */
    template<typename T>
    std::shared_ptr<T> getOptionalService() {
        return getOptionalService<T>("global");
    }
    
    /**
     * @brief Get an optional service from specific namespace
     * @param ns Namespace to search
     * @return Service or nullptr if not found
     */
    template<typename T>
    std::shared_ptr<T> getOptionalService(const std::string& ns) {
        auto service = getServiceInternal(std::type_index(typeid(T)), ns);
        if (!service) {
            return nullptr;
        }
        return std::static_pointer_cast<T>(service);
    }
    
    /**
     * @brief Create a new scope for scoped services (per-request)
     */
    virtual std::shared_ptr<IServiceScope> createScope() = 0;

protected:
    /**
     * @brief Internal service resolution (type-erased)
     * @param type Type index for the requested service
     * @param ns Namespace (e.g., "global", "plugin:auth")
     * @return Shared pointer to void (caller must cast)
     */
    virtual std::shared_ptr<void> getServiceInternal(
        const std::type_index& type, 
        const std::string& ns) = 0;
};

} // namespace http

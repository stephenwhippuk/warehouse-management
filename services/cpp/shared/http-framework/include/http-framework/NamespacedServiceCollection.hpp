#pragma once

#include "ServiceCollection.hpp"
#include "ServiceNamespace.hpp"
#include <string>
#include <functional>

namespace http {

/**
 * @brief Wrapper around ServiceCollection that automatically namespaces registrations
 * 
 * All services registered through this collection go into a specific namespace
 * without the plugin code having to explicitly specify it.
 * 
 * Usage:
 *     NamespacedServiceCollection pluginServices(mainServices, "plugin:auth");
 *     pluginServices.addScoped<IAuthService, AuthService>();
 *     // Service registered as "plugin:auth" namespace, not "global"
 */
class NamespacedServiceCollection {
public:
    /**
     * @brief Create a namespaced collection wrapper
     * @param services Underlying ServiceCollection
     * @param ns Namespace for all registrations in this wrapper (e.g., "plugin:auth")
     */
    explicit NamespacedServiceCollection(ServiceCollection& services, const std::string& ns)
        : services_(services), namespace_(ns) {
        ServiceNamespace::validate(namespace_);
    }

    /**
     * @brief Add a transient service to this namespace
     */
    template<typename TInterface, typename TImplementation>
    void addTransient() {
        services_.addService<TInterface, TImplementation>(
            ServiceLifetime::Transient,
            namespace_,
            ServiceVisibility::Exported
        );
    }

    /**
     * @brief Add a scoped service to this namespace
     */
    template<typename TInterface, typename TImplementation>
    void addScoped() {
        services_.addService<TInterface, TImplementation>(
            ServiceLifetime::Scoped,
            namespace_,
            ServiceVisibility::Exported
        );
    }

    /**
     * @brief Add a singleton service to this namespace
     */
    template<typename TInterface, typename TImplementation>
    void addSingleton() {
        services_.addService<TInterface, TImplementation>(
            ServiceLifetime::Singleton,
            namespace_,
            ServiceVisibility::Exported
        );
    }

    /**
     * @brief Add a service with custom lifetime to this namespace
     */
    template<typename TInterface, typename TImplementation>
    void addService(ServiceLifetime lifetime) {
        services_.addService<TInterface, TImplementation>(
            lifetime,
            namespace_,
            ServiceVisibility::Exported
        );
    }

    /**
     * @brief Add a service with custom factory to this namespace
     */
    template<typename TInterface>
    void addService(
        std::function<std::shared_ptr<TInterface>(IServiceProvider&)> factory,
        ServiceLifetime lifetime = ServiceLifetime::Scoped
    ) {
        services_.addService<TInterface>(
            factory,
            lifetime,
            namespace_,
            ServiceVisibility::Exported
        );
    }

    /**
     * @brief Add service as internal (not visible outside this namespace)
     */
    template<typename TInterface, typename TImplementation>
    void addInternal(ServiceLifetime lifetime = ServiceLifetime::Scoped) {
        services_.addService<TInterface, TImplementation>(
            lifetime,
            namespace_,
            ServiceVisibility::Internal
        );
    }

    /**
     * @brief Get the namespace for this collection
     */
    const std::string& getNamespace() const {
        return namespace_;
    }

    /**
     * @brief Access underlying collection (if needed)
     */
    ServiceCollection& underlying() {
        return services_;
    }

private:
    ServiceCollection& services_;
    std::string namespace_;
};

} // namespace http

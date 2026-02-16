#pragma once

#include "ServiceDescriptor.hpp"
#include "ServiceLifetime.hpp"
#include "ServiceNamespace.hpp"
#include "IServiceProvider.hpp"
#include <memory>
#include <vector>
#include <typeindex>
#include <functional>
#include <string>

namespace http {

/**
 * @brief Service registration API for dependency injection
 * 
 * Register services with different lifetimes (Transient, Scoped, Singleton).
 * Services can be organized by namespace (e.g., per-plugin) to prevent
 * naming conflicts and control visibility.
 * 
 * After registration, call buildServiceProvider() to create the provider.
 */
class ServiceCollection {
public:
    /**
     * @brief Add a transient service to global namespace (new instance every request)
     * @tparam TInterface Service interface type
     * @tparam TImplementation Concrete implementation type
     */
    template<typename TInterface, typename TImplementation>
    void addTransient() {
        addService<TInterface, TImplementation>(ServiceLifetime::Transient);
    }
    
    /**
     * @brief Add a scoped service to global namespace (one instance per request - DEFAULT)
     * @tparam TInterface Service interface type
     * @tparam TImplementation Concrete implementation type
     */
    template<typename TInterface, typename TImplementation>
    void addScoped() {
        addService<TInterface, TImplementation>(ServiceLifetime::Scoped);
    }
    
    /**
     * @brief Add a singleton service to global namespace (one instance for application lifetime)
     * @tparam TInterface Service interface type
     * @tparam TImplementation Concrete implementation type
     */
    template<typename TInterface, typename TImplementation>
    void addSingleton() {
        addService<TInterface, TImplementation>(ServiceLifetime::Singleton);
    }
    
    /**
     * @brief Add a service with explicit lifetime to global namespace
     */
    template<typename TInterface, typename TImplementation>
    void addService(ServiceLifetime lifetime = ServiceLifetime::Scoped) {
        addService<TInterface, TImplementation>(lifetime, std::string(ServiceNamespace::global()));
    }
    
    /**
     * @brief Add a service with custom factory to global namespace
     */
    template<typename TInterface>
    void addService(
        std::function<std::shared_ptr<TInterface>(IServiceProvider&)> factory,
        ServiceLifetime lifetime = ServiceLifetime::Scoped
    ) {
        addService<TInterface>(factory, lifetime, std::string(ServiceNamespace::global()));
    }

    /**
     * @brief Add a service to a specific namespace
     * @tparam TInterface Service interface type
     * @tparam TImplementation Concrete implementation type
     * @param lifetime Service lifetime
     * @param ns Target namespace (default: global)
     * @param visibility Service visibility (default: Exported)
     */
    template<typename TInterface, typename TImplementation>
    void addService(
        ServiceLifetime lifetime,
        const std::string& ns,
        ServiceVisibility visibility = ServiceVisibility::Exported
    ) {
        std::function<std::shared_ptr<TInterface>(IServiceProvider&)> factory =
            [](IServiceProvider& provider) -> std::shared_ptr<TInterface> {
                return std::make_shared<TImplementation>(provider);
            };
        
        addServiceInternal<TInterface>(factory, lifetime, ns, visibility);
    }

    /**
     * @brief Add a service with custom factory to a specific namespace
     * @tparam TInterface Service interface type
     * @param factory Factory function to create instances
     * @param lifetime Service lifetime
     * @param ns Target namespace (default: global)
     * @param visibility Service visibility (default: Exported)
     */
    template<typename TInterface>
    void addService(
        std::function<std::shared_ptr<TInterface>(IServiceProvider&)> factory,
        ServiceLifetime lifetime,
        const std::string& ns,
        ServiceVisibility visibility = ServiceVisibility::Exported
    ) {
        addServiceInternal<TInterface>(factory, lifetime, ns, visibility);
    }

    /**
     * @brief Check if a service is registered in a namespace
     * @tparam TInterface Service interface type
     * @param ns Namespace to check (default: global)
     * @return true if service is registered, false otherwise
     */
    template<typename TInterface>
    bool hasService(const std::string& ns = "global") const {
        auto typeIdx = std::type_index(typeid(TInterface));
        for (const auto& desc : descriptors_) {
            if (desc.getServiceType() == typeIdx && desc.getNamespace() == ns) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get all services in a namespace
     * @param ns Namespace to query (default: global)
     * @return Count of services in that namespace
     */
    int getNamespaceServiceCount(const std::string& ns = "global") const {
        int count = 0;
        for (const auto& desc : descriptors_) {
            if (desc.getNamespace() == ns) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * @brief Build the service provider from registered services
     * @return Service provider instance
     */
    std::shared_ptr<IServiceProvider> buildServiceProvider();
    
    /**
     * @brief Get all registered service descriptors
     */
    const std::vector<ServiceDescriptor>& getDescriptors() const {
        return descriptors_;
    }

private:
    template<typename TInterface>
    void addServiceInternal(
        std::function<std::shared_ptr<TInterface>(IServiceProvider&)> factory,
        ServiceLifetime lifetime,
        const std::string& ns,
        ServiceVisibility visibility
    ) {
        auto typeErasedFactory = [factory](IServiceProvider& provider) -> std::shared_ptr<void> {
            return factory(provider);
        };
        
        // Detect duplicate in same namespace
        auto typeIdx = std::type_index(typeid(TInterface));
        for (const auto& desc : descriptors_) {
            if (desc.getServiceType() == typeIdx && desc.getNamespace() == ns) {
                throw std::runtime_error(
                    "Service already registered in namespace '" + ns + "'"
                );
            }
        }
        
        descriptors_.emplace_back(
            typeIdx,
            typeErasedFactory,
            lifetime,
            ns,
            visibility
        );
    }

    std::vector<ServiceDescriptor> descriptors_;
};

} // namespace http

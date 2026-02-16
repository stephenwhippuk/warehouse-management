#pragma once

#include "ServiceLifetime.hpp"
#include "ServiceNamespace.hpp"
#include "IServiceProvider.hpp"
#include <memory>
#include <functional>
#include <typeindex>
#include <string>

namespace http {

/**
 * @brief Internal descriptor for a registered service
 * 
 * Stores the factory function and metadata for a service registration.
 * Type-erased to allow storage in a heterogeneous container.
 * Includes namespace information for plugin isolation.
 */
class ServiceDescriptor {
public:
    using Factory = std::function<std::shared_ptr<void>(IServiceProvider&)>;
    
    /**
     * @brief Construct a service descriptor
     * @param serviceType Type index of the service interface
     * @param factory Factory function to create service instances
     * @param lifetime Service lifetime strategy
     * @param ns Namespace for service (default: "global")
     * @param visibility Service visibility (default: Exported)
     */
    ServiceDescriptor(
        std::type_index serviceType,
        Factory factory,
        ServiceLifetime lifetime,
        const std::string& ns = "global",
        ServiceVisibility visibility = ServiceVisibility::Exported
    ) : serviceType_(serviceType),
        factory_(factory),
        lifetime_(lifetime),
        namespace_(ns),
        visibility_(visibility) {
        ServiceNamespace::validate(namespace_);
    }
    
    std::type_index getServiceType() const { return serviceType_; }
    const Factory& getFactory() const { return factory_; }
    ServiceLifetime getLifetime() const { return lifetime_; }
    const std::string& getNamespace() const { return namespace_; }
    ServiceVisibility getVisibility() const { return visibility_; }

private:
    std::type_index serviceType_;
    Factory factory_;
    ServiceLifetime lifetime_;
    std::string namespace_;
    ServiceVisibility visibility_;
};

} // namespace http

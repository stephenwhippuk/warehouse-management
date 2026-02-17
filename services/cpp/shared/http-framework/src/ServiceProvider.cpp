#include "http-framework/ServiceCollection.hpp"
#include "http-framework/IServiceProvider.hpp"
#include "http-framework/IServiceScope.hpp"
#include "http-framework/ServiceNamespace.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace http {

// ============================================================================
// ServiceProvider Implementation
// ============================================================================

/**
 * @brief Concrete implementation of IServiceProvider
 * 
 * Thread-safe singleton creation using std::call_once.
 * Maintains separate maps for singletons and scoped services.
 */
class ServiceProvider : public IServiceProvider, public std::enable_shared_from_this<ServiceProvider> {
public:
    explicit ServiceProvider(const std::vector<ServiceDescriptor>& descriptors)
        : descriptors_(descriptors) {}  // Copy the vector!
    
    std::shared_ptr<IServiceScope> createScope() override;

protected:
    std::shared_ptr<void> getServiceInternal(
        const std::type_index& type,
        const std::string& ns) override;

private:
    friend class ServiceScope;
    
    std::vector<ServiceDescriptor> descriptors_;  // COPY, not reference!
    
    // Singleton instances (thread-safe)
    std::unordered_map<std::type_index, std::shared_ptr<void>> singletons_;
    std::unordered_map<std::type_index, std::once_flag> singletonFlags_;
    std::mutex singletonsMutex_;
    
    // Find descriptor by type and namespace
    const ServiceDescriptor* findDescriptor(
        const std::type_index& type, 
        const std::string& ns) const;
    
    // Create service instance using descriptor
    std::shared_ptr<void> createInstance(const ServiceDescriptor& descriptor);
};

// ============================================================================
// ServiceScope Implementation
// ============================================================================

/**
 * @brief Concrete implementation of IServiceScope
 * 
 * Manages scoped service instances that live for the duration of the scope.
 */
class ServiceScope : public IServiceScope {
public:
    explicit ServiceScope(std::shared_ptr<ServiceProvider> rootProvider)
        : rootProvider_(rootProvider),
          scopedProvider_(std::make_unique<ScopedServiceProvider>(
              rootProvider, 
              scopedInstances_)) {}
    
    IServiceProvider& getServiceProvider() override {
        return *scopedProvider_;
    }

private:
    std::shared_ptr<ServiceProvider> rootProvider_;
    
    // Scoped service instances (lifetime tied to this scope)
    std::unordered_map<std::type_index, std::shared_ptr<void>> scopedInstances_;
    
    // Scoped provider that uses the scoped instances map
    class ScopedServiceProvider : public IServiceProvider {
    public:
        ScopedServiceProvider(
            std::shared_ptr<ServiceProvider> rootProvider,
            std::unordered_map<std::type_index, std::shared_ptr<void>>& scopedInstances)
            : rootProvider_(rootProvider),
              scopedInstances_(scopedInstances) {}
        
        std::shared_ptr<IServiceScope> createScope() override {
            // Nested scopes not supported yet
            throw std::runtime_error("Nested scopes are not supported");
        }

    protected:
        std::shared_ptr<void> getServiceInternal(
            const std::type_index& type,
            const std::string& ns) override {
            
            auto descriptor = rootProvider_->findDescriptor(type, ns);
            if (!descriptor) {
                return nullptr;
            }
            
            switch (descriptor->getLifetime()) {
                case ServiceLifetime::Transient:
                    // Always create new instance
                    return descriptor->getFactory()(*this);
                
                case ServiceLifetime::Scoped: {
                    // Reuse scoped instance if exists
                    auto it = scopedInstances_.find(type);
                    if (it != scopedInstances_.end()) {
                        return it->second;
                    }
                    
                    // Create new scoped instance
                    auto instance = descriptor->getFactory()(*this);
                    scopedInstances_[type] = instance;
                    return instance;
                }
                
                case ServiceLifetime::Singleton:
                    // Delegate to root provider for singleton
                    return rootProvider_->getServiceInternal(type, ns);
            }
            
            return nullptr;
        }

    private:
        std::shared_ptr<ServiceProvider> rootProvider_;
        std::unordered_map<std::type_index, std::shared_ptr<void>>& scopedInstances_;
    };
    
    std::unique_ptr<ScopedServiceProvider> scopedProvider_;
};

// ============================================================================
// ServiceProvider Implementation (continued)
// ============================================================================

std::shared_ptr<IServiceScope> ServiceProvider::createScope() {
    return std::make_shared<ServiceScope>(shared_from_this());
}

std::shared_ptr<void> ServiceProvider::getServiceInternal(
    const std::type_index& type,
    const std::string& ns) {
    
    auto descriptor = findDescriptor(type, ns);
    if (!descriptor) {
        return nullptr;
    }
    
    switch (descriptor->getLifetime()) {
        case ServiceLifetime::Transient:
            // Always create new instance
            return descriptor->getFactory()(*this);
        
        case ServiceLifetime::Scoped:
            // Should not be called directly - use scope
            throw std::runtime_error(
                "Scoped service requested from root provider. "
                "Use createScope() and resolve from scoped provider."
            );
        
        case ServiceLifetime::Singleton: {
            // Thread-safe singleton creation using std::call_once
            std::lock_guard<std::mutex> lock(singletonsMutex_);
            
            auto& flag = singletonFlags_[type];
            auto& instance = singletons_[type];
            
            std::call_once(flag, [&]() {
                instance = descriptor->getFactory()(*this);
            });
            
            return instance;
        }
    }
    
    return nullptr;
}

const ServiceDescriptor* ServiceProvider::findDescriptor(
    const std::type_index& type,
    const std::string& ns) const {
    
    // Direct namespace lookup - exact match required
    for (const auto& desc : descriptors_) {
        if (desc.getServiceType() == type && desc.getNamespace() == ns) {
            return &desc;
        }
    }
    
    // If requesting from a plugin namespace and not found, can fall back to global
    // if service is exported (visible across namespaces)
    if (ServiceNamespace::isPluginNamespace(ns)) {
        for (const auto& desc : descriptors_) {
            if (desc.getServiceType() == type && 
                desc.getNamespace() == "global" &&
                desc.getVisibility() == ServiceVisibility::Exported) {
                return &desc;
            }
        }
    }
    
    return nullptr;
}

// ============================================================================
// ServiceCollection Implementation
// ============================================================================

std::shared_ptr<IServiceProvider> ServiceCollection::buildServiceProvider() {
    return std::make_shared<ServiceProvider>(descriptors_);
}

} // namespace http

#pragma once

#include "http-framework/Middleware.hpp"
#include "http-framework/IServiceProvider.hpp"
#include "http-framework/HttpContext.hpp"
#include <memory>

namespace http {

/**
 * @brief Middleware that creates a new service scope for each HTTP request
 * 
 * This middleware:
 * - Creates a new IServiceScope from the root service provider for each request
 * - Stores the scope in HttpContext so it's available to handlers
 * - Ensures scoped services are reused within a single request
 * - Automatically cleans up the scope when the request completes
 * 
 * Must be registered FIRST in the middleware pipeline so it runs for all requests.
 * 
 * Usage:
 * ```cpp
 * auto httpHost = std::make_shared<HttpHost>(port);
 * httpHost->use(std::make_shared<ServiceScopeMiddleware>(serviceProvider));
 * httpHost->use(std::make_shared<ErrorHandlingMiddleware>());
 * // ... other middleware
 * ```
 */
class ServiceScopeMiddleware : public Middleware {
public:
    /**
     * @brief Construct middleware with root service provider
     * @param serviceProvider Root provider for creating scopes
     */
    explicit ServiceScopeMiddleware(std::shared_ptr<IServiceProvider> serviceProvider);

    /**
     * @brief Process request by creating a service scope
     * 
     * Creates a new scope from the root provider and stores it in the HttpContext.
     * The scope is automatically cleaned up when the HttpContext is destroyed.
     * 
     * @param context HTTP request context
     * @param next Callback to next middleware in pipeline
     */
    void process(HttpContext& context, std::function<void()> next) override;

private:
    std::shared_ptr<IServiceProvider> serviceProvider_;
};

} // namespace http

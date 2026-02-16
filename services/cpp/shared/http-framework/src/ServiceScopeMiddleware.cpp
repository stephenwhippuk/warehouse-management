#include "http-framework/ServiceScopeMiddleware.hpp"

namespace http {

ServiceScopeMiddleware::ServiceScopeMiddleware(std::shared_ptr<IServiceProvider> serviceProvider)
    : serviceProvider_(serviceProvider) {
    if (!serviceProvider_) {
        throw std::invalid_argument("ServiceProvider cannot be null");
    }
}

void ServiceScopeMiddleware::process(HttpContext& context, std::function<void()> next) {
    // Create a new scope for this request
    auto scope = serviceProvider_->createScope();
    
    // Store the scope in the context so handlers can access scoped services
    context.setServiceScope(scope);
    
    // Continue with next middleware/handler
    next();
    
    // Scope is automatically cleaned up when method exits and scope goes out of scope
}

} // namespace http

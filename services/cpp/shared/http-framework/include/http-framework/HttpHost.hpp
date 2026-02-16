#pragma once

#include "http-framework/HttpContext.hpp"
#include "http-framework/Middleware.hpp"
#include "http-framework/Router.hpp"
#include "http-framework/ControllerBase.hpp"
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/ServerSocket.h>
#include <memory>
#include <vector>
#include <functional>

namespace http {

// Forward declarations
class IServiceProvider;
class IExceptionFilter;
class ServiceCollection;
class IPlugin;
class HttpHost;

/**
 * @brief HTTP request handler that integrates with framework
 */
class FrameworkRequestHandler : public Poco::Net::HTTPRequestHandler {
public:
    FrameworkRequestHandler(HttpHost* host);
    
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;

private:
    HttpHost* host_;
};

/**
 * @brief Request handler factory for framework
 */
class FrameworkHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    explicit FrameworkHandlerFactory(HttpHost* host);
    
    Poco::Net::HTTPRequestHandler* createRequestHandler(
        const Poco::Net::HTTPServerRequest& request) override;

private:
    HttpHost* host_;
};

/**
 * @brief HTTP Host - Main server class that orchestrates the framework
 * 
 * The HttpHost manages the middleware pipeline, router, and controllers.
 * It integrates with Poco HTTPServer to handle incoming requests.
 * 
 * Example usage:
 * 
 * http::ServiceCollection services;
 * auto provider = services.buildServiceProvider();
 * auto host = HttpHost(8080, provider);
 * 
 * // HttpHost adds ServiceScopeMiddleware + ErrorHandlingMiddleware by default
 * // Add additional middleware
 * host.use(std::make_shared<LoggingMiddleware>());
 * host.use(std::make_shared<AuthenticationMiddleware>());
 * 
 * // Add controllers
 * auto service = std::make_shared<InventoryService>(...);
 * host.addController(std::make_shared<InventoryController>(service));
 * 
 * // Configure server
 * host.setMaxThreads(16);
 * host.setMaxQueued(100);
 * 
 * // Start server
 * host.start();
 */
class HttpHost {
public:
    /**
     * @brief Constructor
        * @param port Server port
        * @param provider Root service provider for DI scope middleware
        * @param host Server host (default: 0.0.0.0)
        * @param exceptionFilter Optional exception filter for error handling
        */
    explicit HttpHost(int port,
                      std::shared_ptr<IServiceProvider> provider,
                      const std::string& host = "0.0.0.0",
                      std::shared_ptr<IExceptionFilter> exceptionFilter = nullptr);

    /**
        * @brief Constructor
     * @param port Server port
     * @param host Server host (default: 0.0.0.0)
     */
    explicit HttpHost(int port, const std::string& host = "0.0.0.0");
    
    /**
     * @brief Destructor - stops server if running
     */
    ~HttpHost();
    
    // Prevent copying
    HttpHost(const HttpHost&) = delete;
    HttpHost& operator=(const HttpHost&) = delete;
    
    /**
     * @brief Add middleware to the pipeline
     * Middleware is executed in the order added
     */
    void use(std::shared_ptr<Middleware> middleware);
    
    /**
     * @brief Add controller to the host
     * All controller routes are registered with the router
     */
    void addController(std::shared_ptr<ControllerBase> controller);
    
    /**
     * @brief Add a single route directly
     */
    void addRoute(const std::string& method, const std::string& pattern, EndpointHandler handler);
    
    /**
     * @brief Set maximum number of threads
     */
    void setMaxThreads(int maxThreads);
    
    /**
     * @brief Set maximum queued requests
     */
    void setMaxQueued(int maxQueued);
    
    /**
     * @brief Set request timeout in seconds
     */
    void setTimeout(int seconds);
    
    /**
     * @brief Start the HTTP server
     * This method blocks until server is stopped
     */
    void start();
    
    /**
     * @brief Stop the HTTP server
     */
    void stop();
    
    /**
     * @brief Check if server is running
     */
    bool isRunning() const;
    
    /**
     * @brief Get the port number
     */
    int getPort() const;
    
    /**
     * @brief Get the host address
     */
    const std::string& getHost() const;
    
    /**
     * @brief Handle a request (called internally by FrameworkRequestHandler)
     */
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response);

    /**
     * @brief Override the exception filter used by error handling middleware
     * @throws std::runtime_error if called while server is running
     */
    void setExceptionFilter(std::shared_ptr<IExceptionFilter> filter);

    /**
     * @brief Register a plugin's services into the service collection
     * @param services Service collection to register into
     * @param plugin Plugin instance
     */
    static void registerPlugin(ServiceCollection& services, IPlugin& plugin);

    /**
     * @brief Add plugin middleware and controllers to this host
     * @param plugin Plugin instance
     * @param provider Service provider for resolving middleware dependencies
     */
    void usePlugin(IPlugin& plugin, IServiceProvider& provider);

private:
    int port_;
    std::string host_;
    int maxThreads_ = 16;
    int maxQueued_ = 100;
    int timeout_ = 60;
    
    MiddlewarePipeline middleware_;
    std::shared_ptr<class ServiceScopeMiddleware> serviceScopeMiddleware_;
    std::shared_ptr<class ErrorHandlingMiddleware> errorHandlingMiddleware_;
    Router router_;
    std::vector<std::shared_ptr<ControllerBase>> controllers_;
    
    std::unique_ptr<Poco::Net::HTTPServer> httpServer_;
    bool running_ = false;
    
    void processRequest(HttpContext& ctx);
    void send404(Poco::Net::HTTPServerResponse& response, const std::string& path);
    void initializeDefaultMiddleware(std::shared_ptr<IServiceProvider> provider,
                                    std::shared_ptr<IExceptionFilter> exceptionFilter);
};

} // namespace http

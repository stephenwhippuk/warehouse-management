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
 * auto host = HttpHost(8080);
 * 
 * // Add middleware
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

private:
    int port_;
    std::string host_;
    int maxThreads_ = 16;
    int maxQueued_ = 100;
    int timeout_ = 60;
    
    MiddlewarePipeline middleware_;
    Router router_;
    std::vector<std::shared_ptr<ControllerBase>> controllers_;
    
    std::unique_ptr<Poco::Net::HTTPServer> httpServer_;
    bool running_ = false;
    
    void processRequest(HttpContext& ctx);
    void send404(Poco::Net::HTTPServerResponse& response, const std::string& path);
};

} // namespace http

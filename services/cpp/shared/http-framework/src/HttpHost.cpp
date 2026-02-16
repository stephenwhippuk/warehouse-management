#include "http-framework/HttpHost.hpp"
#include <Poco/Net/HTTPServerParams.h>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>
#include <iostream>

namespace http {

namespace {

std::atomic_bool g_running{true};

void signalHandler(int) {
    g_running = false;
}

void waitForTerminationRequest() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // anonymous namespace

// ============================================================================
// FrameworkRequestHandler Implementation
// ============================================================================

FrameworkRequestHandler::FrameworkRequestHandler(HttpHost* host) 
    : host_(host) {
}

void FrameworkRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                           Poco::Net::HTTPServerResponse& response) {
    host_->handleRequest(request, response);
}

// ============================================================================
// FrameworkHandlerFactory Implementation
// ============================================================================

FrameworkHandlerFactory::FrameworkHandlerFactory(HttpHost* host) 
    : host_(host) {
}

Poco::Net::HTTPRequestHandler* FrameworkHandlerFactory::createRequestHandler(
    const Poco::Net::HTTPServerRequest& /* request */) {
    return new FrameworkRequestHandler(host_);
}

// ============================================================================
// HttpHost Implementation
// ============================================================================

HttpHost::HttpHost(int port, const std::string& host) 
    : port_(port)
    , host_(host) {
}

HttpHost::~HttpHost() {
    stop();
}

void HttpHost::use(std::shared_ptr<Middleware> middleware) {
    if (running_) {
        throw std::runtime_error("Cannot add middleware while server is running");
    }
    middleware_.use(middleware);
}

void HttpHost::addController(std::shared_ptr<ControllerBase> controller) {
    if (running_) {
        throw std::runtime_error("Cannot add controllers while server is running");
    }
    
    try {
        controllers_.push_back(controller);
        controller->registerRoutes(router_);
    } catch (const std::runtime_error& e) {
        controllers_.pop_back();  // Remove the controller we just added
        throw std::runtime_error(
            "Failed to register controller '" + controller->getBaseRoute() + "': " + 
            std::string(e.what())
        );
    }
}

void HttpHost::addRoute(const std::string& method, const std::string& pattern, EndpointHandler handler) {
    if (running_) {
        throw std::runtime_error("Cannot add routes while server is running");
    }
    router_.addRoute(method, pattern, handler);
}

void HttpHost::setMaxThreads(int maxThreads) {
    maxThreads_ = maxThreads;
}

void HttpHost::setMaxQueued(int maxQueued) {
    maxQueued_ = maxQueued;
}

void HttpHost::setTimeout(int seconds) {
    timeout_ = seconds;
}

void HttpHost::start() {
    if (running_) {
        throw std::runtime_error("Server is already running");
    }
    
    try {
        // Create server socket - bind to specific address
        Poco::Net::SocketAddress socketAddress(host_, port_);
        Poco::Net::ServerSocket socket(socketAddress);
        
        // Configure server parameters
        Poco::Net::HTTPServerParams* params = new Poco::Net::HTTPServerParams;
        params->setMaxQueued(maxQueued_);
        params->setMaxThreads(maxThreads_);
        params->setTimeout(Poco::Timespan(timeout_, 0));
        
        // Create HTTP server
        httpServer_ = std::make_unique<Poco::Net::HTTPServer>(
            new FrameworkHandlerFactory(this),
            socket,
            params
        );
        
        httpServer_->start();
        running_ = true;
        
        std::cout << "HTTP Server started on " << host_ << ":" << port_ << std::endl;
        std::cout << "Registered " << router_.size() << " routes" << std::endl;
        std::cout << "Middleware pipeline: " << middleware_.size() << " middleware" << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        // Wait for termination signal
        waitForTerminationRequest();
        
        stop();
    } catch (const Poco::Exception& e) {
        std::cerr << "Failed to start server: " << e.displayText() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start server: " << e.what() << std::endl;
        throw;
    }
}

void HttpHost::stop() {
    if (httpServer_ && running_) {
        std::cout << "Stopping HTTP server..." << std::endl;
        httpServer_->stop();
        httpServer_.reset();
        running_ = false;
        std::cout << "HTTP server stopped" << std::endl;
    }
}

bool HttpHost::isRunning() const {
    return running_;
}

int HttpHost::getPort() const {
    return port_;
}

const std::string& HttpHost::getHost() const {
    return host_;
}

void HttpHost::handleRequest(Poco::Net::HTTPServerRequest& request,
                            Poco::Net::HTTPServerResponse& response) {
    Poco::URI uri(request.getURI());
    auto queryParams = uri.getQueryParameters();
    
    HttpContext ctx(request, response, queryParams);
    
    // Execute middleware pipeline and route request
    middleware_.execute(ctx, [this, &ctx]() {
        processRequest(ctx);
    });
}

void HttpHost::processRequest(HttpContext& ctx) {
    std::string method = ctx.getMethod();
    std::string path = ctx.getPath();
    
    // Find matching route
    auto route = router_.findRoute(method, path);
    
    if (!route) {
        send404(ctx.response, path);
        return;
    }
    
    // Extract route parameters
    ctx.routeParams = route->extractParameters(path);
    
    // Call endpoint handler
    try {
        std::string responseJson = route->getHandler()(ctx);
        
        // If handler returned content and response not already sent
        if (!responseJson.empty() && ctx.response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
            ctx.sendJson(responseJson);
        } else if (!responseJson.empty()) {
            // Status was set but response not sent yet
            ctx.setHeader("Content-Type", "application/json");
            std::ostream& out = ctx.response.send();
            out << responseJson;
        }
    } catch (const std::exception& e) {
        // If error handling middleware didn't catch it
        ctx.sendError("Internal server error: " + std::string(e.what()),
                     Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void HttpHost::send404(Poco::Net::HTTPServerResponse& response, const std::string& path) {
    response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
    response.setContentType("application/json");
    
    json errorJson = {
        {"error", true},
        {"message", "Route not found"},
        {"path", path},
        {"status", 404}
    };
    
    std::string body = errorJson.dump();
    response.setContentLength(body.length());
    
    std::ostream& out = response.send();
    out << body;
}

} // namespace http

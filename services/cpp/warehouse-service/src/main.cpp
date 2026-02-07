#include "warehouse/Application.hpp"
#include "warehouse/utils/Logger.hpp"
#include <iostream>
#include <exception>
#include <csignal>

using namespace warehouse;

// Global application instance for signal handling
Application* g_app = nullptr;

void signalHandler(int signal) {
    if (g_app) {
        utils::Logger::info("Received signal {}, shutting down...", signal);
        g_app->shutdown();
    }
}

int main(int argc, char** argv) {
    try {
        // Initialize logger
        utils::Logger::init("logs/warehouse-service.log", 
                           utils::Logger::Level::Info, 
                           true);
        
        utils::Logger::info("Starting Warehouse Service...");
        
        // Determine config file
        std::string configFile = "config/application.json";
        if (argc > 1) {
            configFile = argv[1];
        }
        
        // Create and initialize application
        Application& app = Application::instance();
        g_app = &app;
        
        // Setup signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        if (!app.initialize(configFile)) {
            utils::Logger::error("Failed to initialize application");
            return 1;
        }
        
        utils::Logger::info("Warehouse Service initialized successfully");
        utils::Logger::info("Starting HTTP server...");
        
        // Run the application
        app.run();
        
        utils::Logger::info("Warehouse Service stopped");
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        utils::Logger::critical("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        utils::Logger::critical("Unknown fatal error");
        return 1;
    }
}

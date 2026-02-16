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
        g_app->stop();
    }
}

int main(int argc, char** argv) {
    try {
        // Create application
        Application app;
        g_app = &app;
        
        // Setup signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // Run the application (handles initialization internally)
        return app.run(argc, argv);
        
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

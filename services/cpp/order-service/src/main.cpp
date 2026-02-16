#include "order/Application.hpp"
#include "order/utils/Logger.hpp"
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);

void signalHandler(int signal) {
    order::utils::Logger::info("Received signal {}, shutting down...", signal);
    running = false;
}

int main(int argc, char** argv) {
    (void)argc;  // Unused parameter
    (void)argv;  // Unused parameter
    

    try {
        // Create and initialize application
        order::Application app;
        
        if (!app.initialize()) {
            order::utils::Logger::critical("Failed to initialize application");
            return 1;
        }
        
        // Set up signal handling
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // Start application
        app.run();
        
        // Main loop
        order::utils::Logger::info("Order Service is running. Press Ctrl+C to stop.");
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Shutdown
        app.shutdown();
        order::utils::Logger::info("Order Service stopped");
        
        return 0;
    } catch (const std::exception& e) {
        order::utils::Logger::critical("Fatal error: {}", e.what());
        return 1;
    }
}

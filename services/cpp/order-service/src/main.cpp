#include "order/Server.hpp"
#include "order/services/OrderService.hpp"
#include "order/repositories/OrderRepository.hpp"
#include "order/utils/Config.hpp"
#include "order/utils/Logger.hpp"
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);

void signalHandler(int signal) {
    order::utils::Logger::info("Received signal {}, shutting down...", signal);
    running = false;
}

int main(int argc, char** argv) {
    try {
        // Initialize logger
        order::utils::Logger::init("logs/order-service.log",
                                  order::utils::Logger::Level::Info,
                                  true);
        
        order::utils::Logger::info("Order Service starting...");
        
        // Load configuration
        auto& config = order::utils::Config::instance();
        if (!config.load("config/application.json")) {
            order::utils::Logger::warn("Failed to load config file, using defaults");
        }
        
        // Override with environment variables
        const char* portEnv = std::getenv("ORDER_SERVICE_PORT");
        if (portEnv) {
            config.set("server.port", std::atoi(portEnv));
        }
        
        // Create dependencies
        auto repository = std::make_shared<order::repositories::OrderRepository>();
        auto service = std::make_shared<order::services::OrderService>(repository);
        
        // Create and start server
        order::Config serverConfig = config.getServerConfig();
        order::Server server(serverConfig, service);
        
        // Set up signal handling
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        server.start();
        
        order::utils::Logger::info("Order Service is running. Press Ctrl+C to stop.");
        
        // Main loop
        while (running && server.isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        server.stop();
        order::utils::Logger::info("Order Service stopped");
        
        return 0;
    } catch (const std::exception& e) {
        order::utils::Logger::critical("Fatal error: {}", e.what());
        return 1;
    }
}

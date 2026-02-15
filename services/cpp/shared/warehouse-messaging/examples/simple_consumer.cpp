#include "warehouse/messaging/EventConsumer.hpp"
#include "warehouse/messaging/Event.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

using namespace warehouse::messaging;

// Global flag for graceful shutdown
static std::atomic<bool> keepRunning{true};

void signalHandler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << ", shutting down..." << std::endl;
    keepRunning = false;
}

int main() {
    // Setup signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    try {
        // Create consumer (uses environment variables for connection)
        std::vector<std::string> routingKeys = {
            "product.created",
            "product.updated",
            "product.deleted"
        };
        
        auto consumer = EventConsumer::create("example-consumer", routingKeys);
        
        // Register handler for product.created events
        consumer->onEvent("product.created", [](const Event& event) {
            std::cout << "✅ Product Created:" << std::endl;
            std::cout << "   Event ID: " << event.getId() << std::endl;
            std::cout << "   Type: " << event.getType() << std::endl;
            std::cout << "   Data: " << event.getData().dump(2) << std::endl;
        });
        
        // Register handler for product.updated events
        consumer->onEvent("product.updated", [](const Event& event) {
            std::cout << "✏️  Product Updated:" << std::endl;
            std::cout << "   Event ID: " << event.getId() << std::endl;
            std::cout << "   Data: " << event.getData().dump(2) << std::endl;
        });
        
        // Register handler for product.deleted events
        consumer->onEvent("product.deleted", [](const Event& event) {
            std::cout << "🗑️  Product Deleted:" << std::endl;
            std::cout << "   Event ID: " << event.getId() << std::endl;
            std::cout << "   Product ID: " << event.getData()["productId"] << std::endl;
        });
        
        // Register catch-all handler for logging
        consumer->onAnyEvent([](const Event& event) {
            std::cout << "📨 Received event: " << event.getType() 
                      << " (id: " << event.getId() << ")" << std::endl;
        });
        
        // Start consuming
        consumer->start();
        std::cout << "🚀 Consumer started. Listening for events..." << std::endl;
        std::cout << "   Press Ctrl+C to stop" << std::endl;
        
        // Keep running until signal received
        while (keepRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print metrics periodically
            if (consumer->getProcessedCount() % 10 == 0 && consumer->getProcessedCount() > 0) {
                std::cout << "\n📊 Consumer Metrics:" << std::endl;
                std::cout << "   Processed: " << consumer->getProcessedCount() << std::endl;
                std::cout << "   Failed: " << consumer->getFailedCount() << std::endl;
                std::cout << "   Retried: " << consumer->getRetriedCount() << std::endl;
                std::cout << "   Healthy: " << (consumer->isHealthy() ? "Yes" : "No") << std::endl;
            }
        }
        
        // Stop consumer gracefully
        consumer->stop();
        std::cout << "✅ Consumer stopped" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}

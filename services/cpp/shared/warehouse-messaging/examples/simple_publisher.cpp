#include "warehouse/messaging/EventPublisher.hpp"
#include "warehouse/messaging/Event.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using namespace warehouse::messaging;
using json = nlohmann::json;

int main() {
    try {
        // Create publisher (uses environment variables for connection)
        auto publisher = EventPublisher::create("example-publisher");
        
        // Create event data
        json eventData = {
            {"productId", "550e8400-e29b-41d4-a716-446655440000"},
            {"name", "Widget"},
            {"price", 29.99},
            {"quantity", 100}
        };
        
        // Create event
        Event event("product.created", eventData, "example-publisher");
        
        // Publish event (fire-and-forget)
        publisher->publish(event);
        
        std::cout << "✅ Published event: " << event.getId() << std::endl;
        std::cout << "   Type: " << event.getType() << std::endl;
        std::cout << "   Timestamp: " << event.getTimestamp() << std::endl;
        
        // Check publisher metrics
        std::cout << "\n📊 Publisher Metrics:" << std::endl;
        std::cout << "   Published: " << publisher->getPublishedCount() << std::endl;
        std::cout << "   Failed: " << publisher->getFailedCount() << std::endl;
        std::cout << "   Healthy: " << (publisher->isHealthy() ? "Yes" : "No") << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}

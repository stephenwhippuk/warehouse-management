#include <catch2/catch_test_macros.hpp>
#include "warehouse/messaging/Event.hpp"
#include <nlohmann/json.hpp>

using namespace warehouse::messaging;
using json = nlohmann::json;

TEST_CASE("Event creation", "[event]") {
    json data = {{"key", "value"}};
    Event event("test.event", data, "test-service");
    
    REQUIRE(event.getType() == "test.event");
    REQUIRE(event.getSource() == "test-service");
    REQUIRE(event.getData()["key"] == "value");
    REQUIRE(!event.getId().empty());
    REQUIRE(!event.getTimestamp().empty());
}

TEST_CASE("Event with correlation ID", "[event]") {
    json data = {{"key", "value"}};
    Event event = Event("test.event", data, "test-service")
        .withCorrelationId("correlation-123");
    
    REQUIRE(event.getCorrelationId() == "correlation-123");
}

TEST_CASE("Event with metadata", "[event]") {
    json data = {{"key", "value"}};
    Event event = Event("test.event", data, "test-service")
        .withMetadata("userId", "user-123")
        .withMetadata("sessionId", "session-456");
    
    REQUIRE(event.getMetadata("userId").value() == "user-123");
    REQUIRE(event.getMetadata("sessionId").value() == "session-456");
}

TEST_CASE("Event JSON serialization", "[event]") {
    json data = {{"key", "value"}};
    Event event("test.event", data, "test-service");
    
    // Serialize to JSON
    json j = event.toJson();
    REQUIRE(j.contains("eventId"));
    REQUIRE(j.contains("eventType"));
    REQUIRE(j.contains("timestamp"));
    REQUIRE(j.contains("source"));
    REQUIRE(j.contains("data"));
    
    // Deserialize from JSON
    Event restored = Event::fromJson(j);
    REQUIRE(restored.getId() == event.getId());
    REQUIRE(restored.getType() == event.getType());
    REQUIRE(restored.getSource() == event.getSource());
}

TEST_CASE("Event string serialization", "[event]") {
    json data = {{"key", "value"}};
    Event event("test.event", data, "test-service");
    
    // Serialize to string
    std::string str = event.toString();
    REQUIRE(!str.empty());
    
    // Deserialize from string
    Event restored = Event::fromString(str);
    REQUIRE(restored.getId() == event.getId());
    REQUIRE(restored.getType() == event.getType());
}

TEST_CASE("Event immutability", "[event]") {
    json data = {{"key", "value"}};
    Event event1("test.event", data, "test-service");
    Event event2 = event1.withCorrelationId("correlation-123");
    
    // Original event should not have correlation ID
    REQUIRE(event1.getCorrelationId().empty());
    
    // New event should have correlation ID
    REQUIRE(event2.getCorrelationId() == "correlation-123");
}

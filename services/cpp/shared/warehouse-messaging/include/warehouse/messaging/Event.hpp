#pragma once

#include <string>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

namespace warehouse {
namespace messaging {

using json = nlohmann::json;

/**
 * @brief Represents a domain event in the warehouse management system
 * 
 * Events follow a standard structure with metadata and payload.
 * All events are immutable after creation.
 */
class Event {
public:
    /**
     * @brief Construct an event with type and data
     * @param type Event type (e.g., "product.created", "order.shipped")
     * @param data Event payload (JSON object)
     * @param source Optional source service name (auto-detected if empty)
     */
    Event(const std::string& type, const json& data, const std::string& source = "");
    
    /**
     * @brief Construct an event from JSON representation
     * @param j JSON object containing event fields
     * @return Event instance
     * @throws std::invalid_argument if JSON is invalid
     */
    static Event fromJson(const json& j);
    
    /**
     * @brief Construct an event from JSON string
     * @param jsonString JSON string representation
     * @return Event instance
     * @throws std::invalid_argument if JSON is invalid
     */
    static Event fromString(const std::string& jsonString);
    
    // Accessors (const)
    std::string getId() const { return id_; }
    std::string getType() const { return type_; }
    std::string getTimestamp() const { return timestamp_; }
    std::string getSource() const { return source_; }
    std::string getCorrelationId() const { return correlationId_; }
    json getData() const { return data_; }
    
    /**
     * @brief Get optional metadata value
     * @param key Metadata key
     * @return Optional metadata value
     */
    std::optional<std::string> getMetadata(const std::string& key) const;
    
    /**
     * @brief Get all metadata
     * @return Map of all metadata key-value pairs
     */
    const std::map<std::string, std::string>& getAllMetadata() const { return metadata_; }
    
    // Serialization
    json toJson() const;
    std::string toString() const;
    
    // Fluent builder methods (return new Event with modification)
    Event withCorrelationId(const std::string& correlationId) const;
    Event withMetadata(const std::string& key, const std::string& value) const;
    Event withSource(const std::string& source) const;
    
private:
    std::string id_;           // UUID
    std::string type_;         // Event type (routing key)
    std::string timestamp_;    // ISO 8601 timestamp
    std::string source_;       // Source service name
    std::string correlationId_; // Optional correlation ID for tracing
    json data_;                // Event payload
    std::map<std::string, std::string> metadata_;  // Additional metadata
    
    // Helper to generate UUID
    static std::string generateUuid();
    
    // Helper to generate ISO 8601 timestamp
    static std::string generateTimestamp();
};

} // namespace messaging
} // namespace warehouse

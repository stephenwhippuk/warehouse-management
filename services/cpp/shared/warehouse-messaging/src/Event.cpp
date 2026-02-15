#include "warehouse/messaging/Event.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include <stdexcept>

namespace warehouse {
namespace messaging {

Event::Event(const std::string& type, const json& data, const std::string& source)
    : id_(generateUuid())
    , type_(type)
    , timestamp_(generateTimestamp())
    , source_(source.empty() ? "unknown" : source)
    , correlationId_("")
    , data_(data) {
    
    if (type_.empty()) {
        throw std::invalid_argument("Event type cannot be empty");
    }
    
    if (data_.is_null()) {
        throw std::invalid_argument("Event data cannot be null");
    }
}

Event Event::fromJson(const json& j) {
    if (!j.contains("eventType") || !j.contains("data")) {
        throw std::invalid_argument("Invalid event JSON: missing required fields");
    }
    
    Event event(
        j["eventType"].get<std::string>(),
        j["data"],
        j.value("source", "unknown")
    );
    
    // Restore metadata if present
    if (j.contains("eventId")) {
        event.id_ = j["eventId"].get<std::string>();
    }
    
    if (j.contains("timestamp")) {
        event.timestamp_ = j["timestamp"].get<std::string>();
    }
    
    if (j.contains("correlationId")) {
        event.correlationId_ = j["correlationId"].get<std::string>();
    }
    
    if (j.contains("metadata") && j["metadata"].is_object()) {
        for (auto& [key, value] : j["metadata"].items()) {
            if (value.is_string()) {
                event.metadata_[key] = value.get<std::string>();
            }
        }
    }
    
    return event;
}

Event Event::fromString(const std::string& jsonString) {
    try {
        json j = json::parse(jsonString);
        return fromJson(j);
    } catch (const json::exception& e) {
        throw std::invalid_argument(std::string("Failed to parse event JSON: ") + e.what());
    }
}

std::optional<std::string> Event::getMetadata(const std::string& key) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second;
    }
    return std::nullopt;
}

json Event::toJson() const {
    json j = {
        {"eventId", id_},
        {"eventType", type_},
        {"timestamp", timestamp_},
        {"source", source_},
        {"data", data_}
    };
    
    if (!correlationId_.empty()) {
        j["correlationId"] = correlationId_;
    }
    
    if (!metadata_.empty()) {
        json metadataJson = json::object();
        for (const auto& [key, value] : metadata_) {
            metadataJson[key] = value;
        }
        j["metadata"] = metadataJson;
    }
    
    return j;
}

std::string Event::toString() const {
    return toJson().dump();
}

Event Event::withCorrelationId(const std::string& correlationId) const {
    Event copy = *this;
    copy.correlationId_ = correlationId;
    return copy;
}

Event Event::withMetadata(const std::string& key, const std::string& value) const {
    Event copy = *this;
    copy.metadata_[key] = value;
    return copy;
}

Event Event::withSource(const std::string& source) const {
    Event copy = *this;
    copy.source_ = source;
    return copy;
}

std::string Event::generateUuid() {
    // Simple UUID v4 generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex;
    
    for (int i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    
    for (int i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";  // Version 4
    
    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    
    ss << dis2(gen);  // Variant
    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    
    for (int i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    
    return ss.str();
}

std::string Event::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return ss.str();
}

} // namespace messaging
} // namespace warehouse

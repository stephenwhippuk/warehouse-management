#pragma once

#include "warehouse/messaging/Event.hpp"
#include "warehouse/messaging/MessagingConfig.hpp"
#include <memory>
#include <vector>

namespace warehouse {
namespace messaging {

/**
 * @brief Interface for publishing events to a message broker
 * 
 * EventPublisher provides a simple API for publishing domain events.
 * Implementations handle connection management, retries, and error handling.
 */
class EventPublisher {
public:
    virtual ~EventPublisher() = default;
    
    /**
     * @brief Create a publisher with default configuration
     * @param serviceName Name of the publishing service
     * @return Unique pointer to EventPublisher implementation
     */
    static std::unique_ptr<EventPublisher> create(const std::string& serviceName);
    
    /**
     * @brief Create a publisher with custom configuration
     * @param config Publisher configuration
     * @return Unique pointer to EventPublisher implementation
     */
    static std::unique_ptr<EventPublisher> create(const PublisherConfig& config);
    
    /**
     * @brief Publish an event (fire and forget)
     * @param event Event to publish
     * @throws std::runtime_error if publishing fails
     */
    virtual void publish(const Event& event) = 0;
    
    /**
     * @brief Publish an event with confirmation from broker
     * @param event Event to publish
     * @throws std::runtime_error if publishing fails or not confirmed
     * 
     * This is slower but ensures the broker received the message.
     */
    virtual void publishWithConfirmation(const Event& event) = 0;
    
    /**
     * @brief Publish multiple events in a batch
     * @param events Vector of events to publish
     * @throws std::runtime_error if any publish fails
     * 
     * More efficient than publishing events individually.
     */
    virtual void publishBatch(const std::vector<Event>& events) = 0;
    
    /**
     * @brief Check if publisher is healthy and connected
     * @return true if healthy, false otherwise
     */
    virtual bool isHealthy() const = 0;
    
    /**
     * @brief Get count of successfully published events
     * @return Number of events published
     */
    virtual uint64_t getPublishedCount() const = 0;
    
    /**
     * @brief Get count of failed publish attempts
     * @return Number of failed publishes
     */
    virtual uint64_t getFailedCount() const = 0;
};

} // namespace messaging
} // namespace warehouse

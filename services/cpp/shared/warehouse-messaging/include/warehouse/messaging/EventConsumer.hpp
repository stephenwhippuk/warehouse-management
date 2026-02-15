#pragma once

#include "warehouse/messaging/Event.hpp"
#include "warehouse/messaging/MessagingConfig.hpp"
#include <memory>
#include <functional>
#include <vector>

namespace warehouse {
namespace messaging {

/**
 * @brief Callback function type for event handlers
 * 
 * Handler receives an Event and processes it. If the handler throws an exception,
 * the consumer will automatically retry (up to max retries) or send to dead letter queue.
 */
using EventHandler = std::function<void(const Event& event)>;

/**
 * @brief Interface for consuming events from a message broker
 * 
 * EventConsumer provides a simple API for subscribing to and processing domain events.
 * Implementations handle:
 * - Connection management and reconnection
 * - Queue declaration with production-ready settings
 * - Manual acknowledgment with automatic retry
 * - Dead letter queue routing for failed messages
 * - Metrics collection
 */
class EventConsumer {
public:
    virtual ~EventConsumer() = default;
    
    /**
     * @brief Create a consumer with routing keys
     * @param serviceName Name of the consuming service (used for queue naming)
     * @param routingKeys Vector of routing key patterns to bind (e.g., {"product.*", "order.created"})
     * @return Unique pointer to EventConsumer implementation
     * 
     * Queue name will be "{serviceName}-events" by default.
     */
    static std::unique_ptr<EventConsumer> create(
        const std::string& serviceName,
        const std::vector<std::string>& routingKeys
    );
    
    /**
     * @brief Create a consumer with custom configuration
     * @param config Consumer configuration
     * @return Unique pointer to EventConsumer implementation
     */
    static std::unique_ptr<EventConsumer> create(const ConsumerConfig& config);
    
    /**
     * @brief Register a handler for a specific event type
     * @param eventType Event type to handle (e.g., "product.created")
     * @param handler Callback function to process the event
     * 
     * Multiple handlers can be registered for the same event type.
     * They will be called in registration order.
     */
    virtual void onEvent(const std::string& eventType, EventHandler handler) = 0;
    
    /**
     * @brief Register a catch-all handler for any event
     * @param handler Callback function to process any event
     * 
     * Catch-all handlers are called after specific event handlers.
     * Useful for logging, metrics, or generic processing.
     */
    virtual void onAnyEvent(EventHandler handler) = 0;
    
    /**
     * @brief Start consuming events
     * 
     * This method starts the consume loop in a background thread.
     * Returns immediately. Call stop() to gracefully shutdown.
     */
    virtual void start() = 0;
    
    /**
     * @brief Stop consuming events gracefully
     * 
     * Stops accepting new messages and waits for current message to complete.
     * Blocks until consume loop exits.
     */
    virtual void stop() = 0;
    
    /**
     * @brief Check if consumer is currently running
     * @return true if consuming, false otherwise
     */
    virtual bool isRunning() const = 0;
    
    /**
     * @brief Check if consumer is healthy and connected
     * @return true if healthy, false otherwise
     */
    virtual bool isHealthy() const = 0;
    
    /**
     * @brief Get count of successfully processed events
     * @return Number of events processed
     */
    virtual uint64_t getProcessedCount() const = 0;
    
    /**
     * @brief Get count of failed event processing attempts
     * @return Number of processing failures
     */
    virtual uint64_t getFailedCount() const = 0;
    
    /**
     * @brief Get count of retried events
     * @return Number of events that were retried
     */
    virtual uint64_t getRetriedCount() const = 0;
};

} // namespace messaging
} // namespace warehouse

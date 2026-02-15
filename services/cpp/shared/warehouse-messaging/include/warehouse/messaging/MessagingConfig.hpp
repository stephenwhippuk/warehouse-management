#pragma once

#include <string>
#include <vector>

namespace warehouse {
namespace messaging {

/**
 * @brief Base configuration for messaging
 */
struct MessagingConfig {
    // Connection settings
    std::string host = "localhost";
    int port = 5672;
    std::string virtualHost = "/";
    std::string username = "guest";
    std::string password = "guest";
    
    // Service identity
    std::string serviceName;  // e.g., "inventory-service"
    
    // Exchange settings
    std::string exchange = "warehouse.events";
    std::string exchangeType = "topic";
    bool exchangeDurable = true;
    
    /**
     * @brief Load configuration from environment variables
     * 
     * Reads:
     * - RABBITMQ_HOST
     * - RABBITMQ_PORT
     * - RABBITMQ_VHOST
     * - RABBITMQ_USER
     * - RABBITMQ_PASSWORD
     * - SERVICE_NAME (or falls back to serviceName parameter)
     * 
     * @param serviceName Service name to use if SERVICE_NAME env var not set
     * @return MessagingConfig populated from environment
     */
    static MessagingConfig fromEnvironment(const std::string& serviceName = "");
    
    /**
     * @brief Load configuration from JSON file
     * @param path Path to JSON configuration file
     * @return MessagingConfig populated from file
     * @throws std::runtime_error if file cannot be read or parsed
     */
    static MessagingConfig fromFile(const std::string& path);
};

/**
 * @brief Configuration for event consumers
 */
struct ConsumerConfig : public MessagingConfig {
    // Queue configuration
    std::string queuePrefix;               // e.g., "inventory-service" (generates "inventory-service-events")
    std::vector<std::string> routingKeys;  // e.g., {"product.*", "warehouse.updated"}
    
    // Queue properties (production-ready defaults)
    bool queueDurable = true;              // Queue survives broker restart
    bool queueExclusive = false;           // Allow multiple consumers (horizontal scaling)
    bool queueAutoDelete = false;          // Queue persists when no consumers
    
    // Resilience settings
    int maxRetries = 3;                    // Maximum retry attempts before DLQ
    int prefetchCount = 1;                 // Process one message at a time
    bool autoReconnect = true;             // Automatically reconnect on connection loss
    int reconnectDelayMs = 5000;           // Delay before reconnect attempt
    
    // Dead letter queue
    bool enableDLQ = true;                 // Enable dead letter queue for failed messages
    std::string dlxExchange = "warehouse.dlx";  // Dead letter exchange name
    std::string dlqQueue = "warehouse.dlq";     // Dead letter queue name
    
    // Performance settings
    bool useThreadPool = false;            // Use thread pool for parallel processing
    int threadPoolSize = 4;                // Number of worker threads (if useThreadPool=true)
    
    /**
     * @brief Create consumer config with sensible defaults
     * @param serviceName Service name for queue naming
     * @param routingKeys Routing keys to bind
     * @return ConsumerConfig with defaults
     */
    static ConsumerConfig withDefaults(
        const std::string& serviceName,
        const std::vector<std::string>& routingKeys
    );
    
    /**
     * @brief Generate queue name from prefix
     * @return Full queue name (e.g., "inventory-service-events")
     */
    std::string getQueueName() const;
};

/**
 * @brief Configuration for event publishers
 */
struct PublisherConfig : public MessagingConfig {
    // Publisher settings
    bool enableConfirmations = false;      // Wait for broker confirmation (slower but guaranteed)
    int connectionPoolSize = 1;            // Number of connections (for high throughput)
    
    // Message properties
    bool persistentMessages = true;        // Messages survive broker restart
    int messagePriority = 0;               // Message priority (0-9, higher is more important)
    
    // Retry settings
    int maxPublishRetries = 3;             // Max retries if publish fails
    int retryDelayMs = 1000;               // Delay between retry attempts
    
    /**
     * @brief Create publisher config with sensible defaults
     * @param serviceName Service name for identification
     * @return PublisherConfig with defaults
     */
    static PublisherConfig withDefaults(const std::string& serviceName);
};

} // namespace messaging
} // namespace warehouse

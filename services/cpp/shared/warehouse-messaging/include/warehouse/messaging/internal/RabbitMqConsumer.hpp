#ifndef WAREHOUSE_MESSAGING_RABBITMQ_CONSUMER_HPP
#define WAREHOUSE_MESSAGING_RABBITMQ_CONSUMER_HPP

#include "warehouse/messaging/EventConsumer.hpp"
#include "warehouse/messaging/MessagingConfig.hpp"

// Support both old (0.10.0) and new (0.15.0+) header paths
#ifdef __has_include
#if __has_include(<rabbitmq-c/amqp.h>)
#include <rabbitmq-c/amqp.h>
#else
#include <amqp.h>
#endif
#else
#include <rabbitmq-c/amqp.h>
#endif

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>

namespace warehouse {
namespace messaging {
namespace internal {

/**
 * @brief RabbitMQ implementation of EventConsumer with full resilience
 * 
 * Features:
 * - Durable queues (survive broker restart)
 * - Manual ACK (reliable message processing)
 * - Automatic retry with exponential backoff
 * - Dead Letter Queue (DLQ) for failed messages
 * - Auto-reconnection on connection loss
 * - QoS prefetch for load balancing
 * - Thread-safe handler execution
 */
class RabbitMqConsumer : public EventConsumer {
public:
    /**
     * @brief Construct consumer with configuration
     * @param config Consumer configuration
     */
    explicit RabbitMqConsumer(const ConsumerConfig& config);
    
    /**
     * @brief Destructor - stops consumer and closes connection
     */
    ~RabbitMqConsumer() override;
    
    // Disable copy
    RabbitMqConsumer(const RabbitMqConsumer&) = delete;
    RabbitMqConsumer& operator=(const RabbitMqConsumer&) = delete;
    
    // EventConsumer interface
    void onEvent(const std::string& eventType, EventHandler handler) override;
    void onAnyEvent(EventHandler handler) override;
    void start() override;
    void stop() override;
    bool isRunning() const override;
    bool isHealthy() const override;
    uint64_t getProcessedCount() const override;
    uint64_t getFailedCount() const override;
    uint64_t getRetriedCount() const override;

private:
    ConsumerConfig config_;
    amqp_connection_state_t conn_;
    amqp_socket_t* socket_;
    bool connected_;
    std::atomic<bool> running_;
    std::atomic<bool> shouldStop_;
    
    // Handler registry
    std::map<std::string, EventHandler> handlers_;
    EventHandler anyEventHandler_;
    std::mutex handlersMutex_;
    
    // Metrics
    mutable std::atomic<uint64_t> processedCount_;
    mutable std::atomic<uint64_t> failedCount_;
    mutable std::atomic<uint64_t> retriedCount_;
    
    // Thread safety
    mutable std::mutex connectionMutex_;
    std::unique_ptr<std::thread> consumerThread_;
    
    /**
     * @brief Establish connection to RabbitMQ
     */
    void connect();
    
    /**
     * @brief Close connection to RabbitMQ
     */
    void disconnect();
    
    /**
     * @brief Reconnect to RabbitMQ with exponential backoff
     */
    void reconnect();
    
    /**
     * @brief Declare exchange
     */
    void declareExchange();
    
    /**
     * @brief Declare queue with DLQ configuration
     */
    void declareQueue();
    
    /**
     * @brief Bind queue to exchange with routing keys
     */
    void bindQueue();
    
    /**
     * @brief Set QoS (prefetch count)
     */
    void setQos();
    
    /**
     * @brief Main consumer loop (runs in background thread)
     */
    void consumeLoop();
    
    /**
     * @brief Process a single message
     * @param envelope Message envelope
     */
    void processMessage(const amqp_envelope_t& envelope);
    
    /**
     * @brief Handle message processing success
     * @param deliveryTag Message delivery tag
     */
    void ackMessage(uint64_t deliveryTag);
    
    /**
     * @brief Handle message processing failure
     * @param deliveryTag Message delivery tag
     * @param requeue Whether to requeue the message
     */
    void nackMessage(uint64_t deliveryTag, bool requeue);
    
    /**
     * @brief Get retry count from message headers
     * @param envelope Message envelope
     * @return Number of previous retries
     */
    int getRetryCount(const amqp_envelope_t& envelope);
    
    /**
     * @brief Check AMQP RPC reply for errors
     * @param reply AMQP RPC reply
     * @param context Context string for error messages
     * @throws std::runtime_error if reply contains error
     */
    void checkReply(const amqp_rpc_reply_t& reply, const std::string& context);
};

} // namespace internal
} // namespace messaging
} // namespace warehouse

#endif // WAREHOUSE_MESSAGING_RABBITMQ_CONSUMER_HPP

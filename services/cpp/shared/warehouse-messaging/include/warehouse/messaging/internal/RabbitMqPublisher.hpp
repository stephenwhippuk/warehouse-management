#ifndef WAREHOUSE_MESSAGING_RABBITMQ_PUBLISHER_HPP
#define WAREHOUSE_MESSAGING_RABBITMQ_PUBLISHER_HPP

#include "warehouse/messaging/EventPublisher.hpp"
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
#include <atomic>
#include <mutex>

namespace warehouse {
namespace messaging {
namespace internal {

/**
 * @brief RabbitMQ implementation of EventPublisher
 * 
 * Handles connection management, publishing with retry logic,
 * and optional publisher confirms for reliability.
 */
class RabbitMqPublisher : public EventPublisher {
public:
    /**
     * @brief Construct publisher with configuration
     * @param config Publisher configuration
     */
    explicit RabbitMqPublisher(const PublisherConfig& config);
    
    /**
     * @brief Destructor - closes connection
     */
    ~RabbitMqPublisher() override;
    
    // Disable copy
    RabbitMqPublisher(const RabbitMqPublisher&) = delete;
    RabbitMqPublisher& operator=(const RabbitMqPublisher&) = delete;
    
    // EventPublisher interface
    void publish(const Event& event) override;
    void publishWithConfirmation(const Event& event) override;
    void publishBatch(const std::vector<Event>& events) override;
    bool isHealthy() const override;
    uint64_t getPublishedCount() const override;
    uint64_t getFailedCount() const override;

private:
    PublisherConfig config_;
    amqp_connection_state_t conn_;
    amqp_socket_t* socket_;
    bool connected_;
    
    // Metrics
    mutable std::atomic<uint64_t> publishedCount_;
    mutable std::atomic<uint64_t> failedCount_;
    
    // Thread safety
    mutable std::mutex connectionMutex_;
    
    /**
     * @brief Establish connection to RabbitMQ
     */
    void connect();
    
    /**
     * @brief Close connection to RabbitMQ
     */
    void disconnect();
    
    /**
     * @brief Reconnect to RabbitMQ
     */
    void reconnect();
    
    /**
     * @brief Declare exchange
     */
    void declareExchange();
    
    /**
     * @brief Publish event with retry logic
     * @param event Event to publish
     * @param waitForConfirm Wait for broker confirmation
     */
    void publishInternal(const Event& event, bool waitForConfirm);
    
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

#endif // WAREHOUSE_MESSAGING_RABBITMQ_PUBLISHER_HPP

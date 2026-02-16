#include "warehouse/messaging/internal/RabbitMqConsumer.hpp"

// Support both old (0.10.0) and new (0.15.0+) header paths
#ifdef __has_include
#if __has_include(<rabbitmq-c/tcp_socket.h>)
#include <rabbitmq-c/tcp_socket.h>
#else
#include <amqp_tcp_socket.h>
#endif
#else
#include <rabbitmq-c/tcp_socket.h>
#endif

#ifdef __has_include
#if __has_include(<rabbitmq-c/framing.h>)
#include <rabbitmq-c/framing.h>
#else
#include <amqp_framing.h>
#endif
#else
#include <rabbitmq-c/framing.h>
#endif

#include <stdexcept>
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>

namespace warehouse {
namespace messaging {
namespace internal {

RabbitMqConsumer::RabbitMqConsumer(const ConsumerConfig& config)
    : config_(config)
    , conn_(nullptr)
    , socket_(nullptr)
    , connected_(false)
    , running_(false)
    , shouldStop_(false)
    , processedCount_(0)
    , failedCount_(0)
    , retriedCount_(0) {
    // Constructor initializes state, actual connection happens in start()
}

RabbitMqConsumer::~RabbitMqConsumer() {
    stop();
    disconnect();
}

void RabbitMqConsumer::connect() {
    std::lock_guard<std::mutex> lock(connectionMutex_);
    
    if (connected_) {
        return;
    }
    
    // Create connection
    conn_ = amqp_new_connection();
    if (!conn_) {
        throw std::runtime_error("Failed to create AMQP connection");
    }
    
    // Create TCP socket
    socket_ = amqp_tcp_socket_new(conn_);
    if (!socket_) {
        amqp_destroy_connection(conn_);
        throw std::runtime_error("Failed to create TCP socket");
    }
    
    // Open socket
    int status = amqp_socket_open(socket_, config_.host.c_str(), config_.port);
    if (status != AMQP_STATUS_OK) {
        amqp_destroy_connection(conn_);
        throw std::runtime_error("Failed to open socket: " + std::string(amqp_error_string2(status)));
    }
    
    // Login
    amqp_rpc_reply_t reply = amqp_login(
        conn_,
        config_.virtualHost.c_str(),
        AMQP_DEFAULT_MAX_CHANNELS,
        AMQP_DEFAULT_FRAME_SIZE,
        0,  // No heartbeat
        AMQP_SASL_METHOD_PLAIN,
        config_.username.c_str(),
        config_.password.c_str()
    );
    checkReply(reply, "Login");
    
    // Open channel
    amqp_channel_open(conn_, 1);
    reply = amqp_get_rpc_reply(conn_);
    checkReply(reply, "Channel open");
    
    // Declare exchange
    declareExchange();
    
    // Declare queue with DLQ
    declareQueue();
    
    // Bind queue to exchange
    bindQueue();
    
    // Set QoS
    setQos();
    
    connected_ = true;
    spdlog::info("[{}] Connected to RabbitMQ: {}:{}", 
                 config_.serviceName, config_.host, config_.port);
}

void RabbitMqConsumer::disconnect() {
    std::lock_guard<std::mutex> lock(connectionMutex_);
    
    if (!connected_) {
        return;
    }
    
    // Close channel
    amqp_channel_close(conn_, 1, AMQP_REPLY_SUCCESS);
    
    // Close connection
    amqp_connection_close(conn_, AMQP_REPLY_SUCCESS);
    
    // Destroy connection
    amqp_destroy_connection(conn_);
    
    connected_ = false;
    spdlog::info("[{}] Disconnected from RabbitMQ", config_.serviceName);
}

void RabbitMqConsumer::reconnect() {
    spdlog::warn("[{}] Reconnecting to RabbitMQ...", config_.serviceName);
    disconnect();
    
    int attempt = 0;
    int maxAttempts = 10;
    int delayMs = config_.reconnectDelayMs;
    
    while (attempt < maxAttempts && !shouldStop_) {
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            connect();
            spdlog::info("[{}] Reconnected successfully", config_.serviceName);
            return;
        } catch (const std::exception& e) {
            attempt++;
            spdlog::error("[{}] Reconnection attempt {}/{} failed: {}",
                         config_.serviceName, attempt, maxAttempts, e.what());
            
            // Exponential backoff (double delay, max 60s)
            delayMs = std::min(delayMs * 2, 60000);
        }
    }
    
    throw std::runtime_error("Failed to reconnect after " + std::to_string(maxAttempts) + " attempts");
}

void RabbitMqConsumer::declareExchange() {
    amqp_bytes_t exchangeBytes = amqp_cstring_bytes(config_.exchange.c_str());
    amqp_bytes_t exchangeTypeBytes = amqp_cstring_bytes(config_.exchangeType.c_str());
    
    amqp_exchange_declare(
        conn_,
        1,  // channel
        exchangeBytes,
        exchangeTypeBytes,
        0,  // passive
        config_.exchangeDurable ? 1 : 0,
        0,  // auto_delete
        0,  // internal
        amqp_empty_table
    );
    
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn_);
    checkReply(reply, "Exchange declare");
}

void RabbitMqConsumer::declareQueue() {
    std::string queueName = config_.getQueueName();
    amqp_bytes_t queueBytes = amqp_cstring_bytes(queueName.c_str());
    
    // Prepare queue arguments (for DLQ)
    amqp_table_t queueArgs;
    amqp_table_entry_t entries[2];
    int numEntries = 0;
    
    if (config_.enableDLQ) {
        // Set dead-letter exchange
        entries[numEntries].key = amqp_cstring_bytes("x-dead-letter-exchange");
        entries[numEntries].value.kind = AMQP_FIELD_KIND_UTF8;
        entries[numEntries].value.value.bytes = amqp_cstring_bytes(config_.dlxExchange.c_str());
        numEntries++;
        
        // Set dead-letter routing key
        entries[numEntries].key = amqp_cstring_bytes("x-dead-letter-routing-key");
        entries[numEntries].value.kind = AMQP_FIELD_KIND_UTF8;
        entries[numEntries].value.value.bytes = amqp_cstring_bytes("dlq");
        numEntries++;
    }
    
    queueArgs.num_entries = numEntries;
    queueArgs.entries = numEntries > 0 ? entries : nullptr;
    
    // Declare main queue
    amqp_queue_declare(
        conn_,
        1,  // channel
        queueBytes,
        0,  // passive
        config_.queueDurable ? 1 : 0,
        config_.queueExclusive ? 1 : 0,
        config_.queueAutoDelete ? 1 : 0,
        queueArgs
    );
    
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn_);
    checkReply(reply, "Queue declare");
    
    // Declare DLX and DLQ if enabled
    if (config_.enableDLQ) {
        // Declare dead-letter exchange
        amqp_bytes_t dlxBytes = amqp_cstring_bytes(config_.dlxExchange.c_str());
        amqp_bytes_t directBytes = amqp_cstring_bytes("direct");
        
        amqp_exchange_declare(
            conn_,
            1,
            dlxBytes,
            directBytes,
            0,  // passive
            1,  // durable
            0,  // auto_delete
            0,  // internal
            amqp_empty_table
        );
        
        reply = amqp_get_rpc_reply(conn_);
        checkReply(reply, "DLX declare");
        
        // Declare dead-letter queue
        amqp_bytes_t dlqBytes = amqp_cstring_bytes(config_.dlqQueue.c_str());
        
        amqp_queue_declare(
            conn_,
            1,
            dlqBytes,
            0,  // passive
            1,  // durable
            0,  // exclusive
            0,  // auto_delete
            amqp_empty_table
        );
        
        reply = amqp_get_rpc_reply(conn_);
        checkReply(reply, "DLQ declare");
        
        // Bind DLQ to DLX
        amqp_bytes_t dlqRoutingKey = amqp_cstring_bytes("dlq");
        
        amqp_queue_bind(
            conn_,
            1,
            dlqBytes,
            dlxBytes,
            dlqRoutingKey,
            amqp_empty_table
        );
        
        reply = amqp_get_rpc_reply(conn_);
        checkReply(reply, "DLQ bind");
    }
}

void RabbitMqConsumer::bindQueue() {
    std::string queueName = config_.getQueueName();
    amqp_bytes_t queueBytes = amqp_cstring_bytes(queueName.c_str());
    amqp_bytes_t exchangeBytes = amqp_cstring_bytes(config_.exchange.c_str());
    
    // Bind queue to exchange for each routing key
    for (const auto& routingKey : config_.routingKeys) {
        amqp_bytes_t routingKeyBytes = amqp_cstring_bytes(routingKey.c_str());
        
        amqp_queue_bind(
            conn_,
            1,
            queueBytes,
            exchangeBytes,
            routingKeyBytes,
            amqp_empty_table
        );
        
        amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn_);
        checkReply(reply, "Queue bind for routing key: " + routingKey);
        
        spdlog::debug("[{}] Bound queue {} to exchange {} with routing key: {}",
                     config_.serviceName, queueName, config_.exchange, routingKey);
    }
}

void RabbitMqConsumer::setQos() {
    amqp_basic_qos(
        conn_,
        1,  // channel
        0,  // prefetch_size (0 = no limit)
        config_.prefetchCount,
        0   // global (0 = apply to channel)
    );
    
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn_);
    checkReply(reply, "Set QoS");
}

void RabbitMqConsumer::onEvent(const std::string& eventType, EventHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_[eventType] = handler;
    spdlog::debug("[{}] Registered handler for event type: {}", config_.serviceName, eventType);
}

void RabbitMqConsumer::onAnyEvent(EventHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    anyEventHandler_ = handler;
    spdlog::debug("[{}] Registered catch-all event handler", config_.serviceName);
}

void RabbitMqConsumer::start() {
    if (running_) {
        spdlog::warn("[{}] Consumer already running", config_.serviceName);
        return;
    }
    
    shouldStop_ = false;
    
    // Connect to RabbitMQ
    connect();
    
    // Start consumer loop in background thread
    consumerThread_ = std::make_unique<std::thread>(&RabbitMqConsumer::consumeLoop, this);
    
    running_ = true;
    spdlog::info("[{}] Consumer started", config_.serviceName);
}

void RabbitMqConsumer::stop() {
    if (!running_) {
        return;
    }
    
    spdlog::info("[{}] Stopping consumer...", config_.serviceName);
    shouldStop_ = true;
    
    // Wait for consumer thread to finish
    if (consumerThread_ && consumerThread_->joinable()) {
        consumerThread_->join();
    }
    
    running_ = false;
    spdlog::info("[{}] Consumer stopped", config_.serviceName);
}

bool RabbitMqConsumer::isRunning() const {
    return running_.load();
}

bool RabbitMqConsumer::isHealthy() const {
    std::lock_guard<std::mutex> lock(connectionMutex_);
    return connected_ && running_;
}

uint64_t RabbitMqConsumer::getProcessedCount() const {
    return processedCount_.load();
}

uint64_t RabbitMqConsumer::getFailedCount() const {
    return failedCount_.load();
}

uint64_t RabbitMqConsumer::getRetriedCount() const {
    return retriedCount_.load();
}

void RabbitMqConsumer::consumeLoop() {
    std::string queueName = config_.getQueueName();
    amqp_bytes_t queueBytes = amqp_cstring_bytes(queueName.c_str());
    
    // Start consuming
    amqp_basic_consume(
        conn_,
        1,  // channel
        queueBytes,
        amqp_empty_bytes,
        0,  // no_local
        0,  // no_ack (manual ACK)
        config_.queueExclusive ? 1 : 0,
        amqp_empty_table
    );
    
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn_);
    checkReply(reply, "Basic consume");
    
    spdlog::info("[{}] Consuming from queue: {}", config_.serviceName, queueName);
    
    // Main consume loop
    while (!shouldStop_) {
        amqp_envelope_t envelope;
        struct timeval timeout;
        timeout.tv_sec = 1;  // 1 second timeout
        timeout.tv_usec = 0;
        
        amqp_rpc_reply_t res = amqp_consume_message(conn_, &envelope, &timeout, 0);
        
        if (res.reply_type == AMQP_RESPONSE_NORMAL) {
            // Process message
            processMessage(envelope);
            amqp_destroy_envelope(&envelope);
        } else if (res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
            if (res.library_error == AMQP_STATUS_TIMEOUT) {
                // Timeout is normal, continue
                continue;
            } else if (res.library_error == AMQP_STATUS_CONNECTION_CLOSED ||
                      res.library_error == AMQP_STATUS_SOCKET_ERROR) {
                // Connection lost, try to reconnect
                spdlog::error("[{}] Connection lost: {}",
                            config_.serviceName, amqp_error_string2(res.library_error));
                
                if (config_.autoReconnect && !shouldStop_) {
                    try {
                        reconnect();
                    } catch (const std::exception& e) {
                        spdlog::error("[{}] Failed to reconnect: {}", config_.serviceName, e.what());
                        break;
                    }
                } else {
                    break;
                }
            } else {
                spdlog::error("[{}] Consume message error: {}",
                            config_.serviceName, amqp_error_string2(res.library_error));
                break;
            }
        } else {
            spdlog::error("[{}] Unexpected response type: {}", config_.serviceName, static_cast<int>(res.reply_type));
            break;
        }
    }
    
    spdlog::info("[{}] Exiting consume loop", config_.serviceName);
}

void RabbitMqConsumer::processMessage(const amqp_envelope_t& envelope) {
    try {
        // Parse event from message body
        std::string payload(
            static_cast<char*>(envelope.message.body.bytes),
            envelope.message.body.len
        );
        
        Event event = Event::fromString(payload);
        
        // Get retry count
        int retryCount = getRetryCount(envelope);
        
        spdlog::debug("[{}] Processing event: {} (id: {}, retry: {})",
                     config_.serviceName, event.getType(), event.getId(), retryCount);
        
        // Find and execute handler
        bool handled = false;
        
        {
            std::lock_guard<std::mutex> lock(handlersMutex_);
            
            // Try specific handler first
            auto it = handlers_.find(event.getType());
            if (it != handlers_.end()) {
                try {
                    it->second(event);
                    handled = true;
                } catch (const std::exception& e) {
                    spdlog::error("[{}] Handler exception for {}: {}",
                                config_.serviceName, event.getType(), e.what());
                    
                    // Check if we should retry
                    if (retryCount < config_.maxRetries) {
                        spdlog::warn("[{}] Retrying message (attempt {}/{})",
                                   config_.serviceName, retryCount + 1, config_.maxRetries);
                        retriedCount_++;
                        nackMessage(envelope.delivery_tag, true);  // Requeue for retry
                        return;
                    } else {
                        spdlog::error("[{}] Max retries exceeded, sending to DLQ",
                                    config_.serviceName);
                        failedCount_++;
                        nackMessage(envelope.delivery_tag, false);  // Don't requeue, goes to DLQ
                        return;
                    }
                }
            }
            
            // Try catch-all handler if no specific handler
            if (!handled && anyEventHandler_) {
                try {
                    anyEventHandler_(event);
                    handled = true;
                } catch (const std::exception& e) {
                    spdlog::error("[{}] Catch-all handler exception: {}",
                                config_.serviceName, e.what());
                }
            }
        }
        
        if (handled) {
            // ACK message on success
            ackMessage(envelope.delivery_tag);
            processedCount_++;
        } else {
            spdlog::warn("[{}] No handler found for event type: {}",
                        config_.serviceName, event.getType());
            // ACK anyway to avoid reprocessing
            ackMessage(envelope.delivery_tag);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("[{}] Error processing message: {}", config_.serviceName, e.what());
        failedCount_++;
        nackMessage(envelope.delivery_tag, false);  // Don't requeue malformed messages
    }
}

void RabbitMqConsumer::ackMessage(uint64_t deliveryTag) {
    std::lock_guard<std::mutex> lock(connectionMutex_);
    
    if (!connected_) {
        return;
    }
    
    int status = amqp_basic_ack(conn_, 1, deliveryTag, 0);
    if (status != AMQP_STATUS_OK) {
        spdlog::error("[{}] Failed to ACK message: {}",
                     config_.serviceName, amqp_error_string2(status));
    }
}

void RabbitMqConsumer::nackMessage(uint64_t deliveryTag, bool requeue) {
    std::lock_guard<std::mutex> lock(connectionMutex_);
    
    if (!connected_) {
        return;
    }
    
    int status = amqp_basic_nack(conn_, 1, deliveryTag, 0, requeue ? 1 : 0);
    if (status != AMQP_STATUS_OK) {
        spdlog::error("[{}] Failed to NACK message: {}",
                     config_.serviceName, amqp_error_string2(status));
    }
}

int RabbitMqConsumer::getRetryCount(const amqp_envelope_t& envelope) {
    // Check if message has x-death header (indicates previous rejections)
    const amqp_table_t* headers = &envelope.message.properties.headers;
    
    if (!(envelope.message.properties._flags & AMQP_BASIC_HEADERS_FLAG)) {
        return 0;
    }
    
    for (size_t i = 0; i < static_cast<size_t>(headers->num_entries); i++) {
        if (std::string((char*)headers->entries[i].key.bytes, headers->entries[i].key.len) == "x-death") {
            if (headers->entries[i].value.kind == AMQP_FIELD_KIND_ARRAY) {
                amqp_array_t* deathArray = &headers->entries[i].value.value.array;
                return deathArray->num_entries;  // Number of deaths = retry count
            }
        }
    }
    
    return 0;
}

void RabbitMqConsumer::checkReply(const amqp_rpc_reply_t& reply, const std::string& context) {
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::string error = context + " failed: ";
        
        switch (reply.reply_type) {
            case AMQP_RESPONSE_NONE:
                error += "Missing RPC reply";
                break;
            case AMQP_RESPONSE_LIBRARY_EXCEPTION:
                error += amqp_error_string2(reply.library_error);
                break;
            case AMQP_RESPONSE_SERVER_EXCEPTION:
                if (reply.reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
                    amqp_connection_close_t* m = (amqp_connection_close_t*)reply.reply.decoded;
                    error += "Connection closed: " + std::string((char*)m->reply_text.bytes, m->reply_text.len);
                } else if (reply.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
                    amqp_channel_close_t* m = (amqp_channel_close_t*)reply.reply.decoded;
                    error += "Channel closed: " + std::string((char*)m->reply_text.bytes, m->reply_text.len);
                } else {
                    error += "Unknown server error";
                }
                break;
            default:
                error += "Unknown error";
        }
        
        throw std::runtime_error(error);
    }
}

} // namespace internal
} // namespace messaging
} // namespace warehouse

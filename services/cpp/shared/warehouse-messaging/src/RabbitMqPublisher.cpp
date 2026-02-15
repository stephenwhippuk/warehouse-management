#include "warehouse/messaging/internal/RabbitMqPublisher.hpp"

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

RabbitMqPublisher::RabbitMqPublisher(const PublisherConfig& config)
    : config_(config)
    , conn_(nullptr)
    , socket_(nullptr)
    , connected_(false)
    , publishedCount_(0)
    , failedCount_(0) {
    
    connect();
}

RabbitMqPublisher::~RabbitMqPublisher() {
    disconnect();
}

void RabbitMqPublisher::connect() {
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
    
    // Enable publisher confirms if requested
    if (config_.enableConfirmations) {
        amqp_confirm_select(conn_, 1);
        reply = amqp_get_rpc_reply(conn_);
        checkReply(reply, "Enable publisher confirms");
    }
    
    connected_ = true;
    spdlog::info("[{}] Connected to RabbitMQ: {}:{}", 
                 config_.serviceName, config_.host, config_.port);
}

void RabbitMqPublisher::disconnect() {
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

void RabbitMqPublisher::reconnect() {
    spdlog::warn("[{}] Reconnecting to RabbitMQ...", config_.serviceName);
    disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(config_.retryDelayMs));
    connect();
}

void RabbitMqPublisher::declareExchange() {
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

void RabbitMqPublisher::publish(const Event& event) {
    publishInternal(event, false);
}

void RabbitMqPublisher::publishWithConfirmation(const Event& event) {
    publishInternal(event, true);
}

void RabbitMqPublisher::publishBatch(const std::vector<Event>& events) {
    for (const auto& event : events) {
        publishInternal(event, false);
    }
}

void RabbitMqPublisher::publishInternal(const Event& event, bool waitForConfirm) {
    std::string payload = event.toString();
    int attempts = 0;
    bool success = false;
    
    while (attempts < config_.maxPublishRetries && !success) {
        try {
            std::lock_guard<std::mutex> lock(connectionMutex_);
            
            if (!connected_) {
                throw std::runtime_error("Not connected to RabbitMQ");
            }
            
            // Prepare message properties
            amqp_basic_properties_t props;
            props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | 
                          AMQP_BASIC_DELIVERY_MODE_FLAG |
                          AMQP_BASIC_MESSAGE_ID_FLAG |
                          AMQP_BASIC_TIMESTAMP_FLAG;
            
            props.content_type = amqp_cstring_bytes("application/json");
            props.delivery_mode = config_.persistentMessages ? 2 : 1;  // 2 = persistent
            props.message_id = amqp_cstring_bytes(event.getId().c_str());
            props.timestamp = static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count() / 1000000
            );
            
            // Add correlation ID if present
            if (!event.getCorrelationId().empty()) {
                props._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
                props.correlation_id = amqp_cstring_bytes(event.getCorrelationId().c_str());
            }
            
            // Publish message
            amqp_bytes_t exchangeBytes = amqp_cstring_bytes(config_.exchange.c_str());
            amqp_bytes_t routingKeyBytes = amqp_cstring_bytes(event.getType().c_str());
            amqp_bytes_t messageBytes = amqp_bytes_malloc_dup(amqp_cstring_bytes(payload.c_str()));
            
            int status = amqp_basic_publish(
                conn_,
                1,  // channel
                exchangeBytes,
                routingKeyBytes,
                0,  // mandatory
                0,  // immediate
                &props,
                messageBytes
            );
            
            amqp_bytes_free(messageBytes);
            
            if (status != AMQP_STATUS_OK) {
                throw std::runtime_error("Publish failed: " + std::string(amqp_error_string2(status)));
            }
            
            // Wait for confirmation if requested
            if (waitForConfirm && config_.enableConfirmations) {
                if (!amqp_simple_wait_frame_noblock(conn_, nullptr, nullptr)) {
                    throw std::runtime_error("Failed to receive publisher confirmation");
                }
            }
            
            success = true;
            publishedCount_++;
            
            spdlog::debug("[{}] Published event: {} (id: {})",
                         config_.serviceName, event.getType(), event.getId());
            
        } catch (const std::exception& e) {
            attempts++;
            spdlog::error("[{}] Publish attempt {}/{} failed: {}",
                         config_.serviceName, attempts, config_.maxPublishRetries, e.what());
            
            if (attempts < config_.maxPublishRetries) {
                try {
                    reconnect();
                } catch (const std::exception& reconnectEx) {
                    spdlog::error("[{}] Reconnection failed: {}",
                                 config_.serviceName, reconnectEx.what());
                }
            }
        }
    }
    
    if (!success) {
        failedCount_++;
        throw std::runtime_error("Failed to publish event after " + 
                               std::to_string(config_.maxPublishRetries) + " attempts");
    }
}

bool RabbitMqPublisher::isHealthy() const {
    std::lock_guard<std::mutex> lock(connectionMutex_);
    return connected_;
}

uint64_t RabbitMqPublisher::getPublishedCount() const {
    return publishedCount_.load();
}

uint64_t RabbitMqPublisher::getFailedCount() const {
    return failedCount_.load();
}

void RabbitMqPublisher::checkReply(const amqp_rpc_reply_t& reply, const std::string& context) {
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

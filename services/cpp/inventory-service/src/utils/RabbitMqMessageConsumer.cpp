#include "inventory/utils/RabbitMqMessageConsumer.hpp"
#include "inventory/utils/Logger.hpp"
#include <amqp_framing.h>
#include <cstring>

namespace inventory {
namespace utils {

RabbitMqMessageConsumer::RabbitMqMessageConsumer(const Config& config)
    : config_(config)
    , connection_(nullptr)
    , socket_(nullptr)
    , channel_(1)
    , running_(false) {
}

RabbitMqMessageConsumer::~RabbitMqMessageConsumer() {
    stopConsuming();
    close();
}

void RabbitMqMessageConsumer::connect() {
    // Create connection
    connection_ = amqp_new_connection();
    socket_ = amqp_tcp_socket_new(connection_);
    if (!socket_) {
        throw std::runtime_error("Failed to create RabbitMQ TCP socket");
    }

    // Open socket
    int status = amqp_socket_open(socket_, config_.host.c_str(), std::stoi(config_.port));
    if (status != AMQP_STATUS_OK) {
        throw std::runtime_error("Failed to open RabbitMQ socket: " + std::string(amqp_error_string2(status)));
    }

    // Login
    amqp_rpc_reply_t reply = amqp_login(
        connection_,
        config_.virtual_host.c_str(),
        0,                              // channel_max
        131072,                         // frame_max
        0,                              // heartbeat
        AMQP_SASL_METHOD_PLAIN,
        config_.username.c_str(),
        config_.password.c_str()
    );
    
    if (!checkAmqpStatus(reply, "Login")) {
        throw std::runtime_error("Failed to login to RabbitMQ");
    }

    // Open channel
    amqp_channel_open(connection_, channel_);
    reply = amqp_get_rpc_reply(connection_);
    if (!checkAmqpStatus(reply, "Channel open")) {
        throw std::runtime_error("Failed to open RabbitMQ channel");
    }

    // Declare queue (exclusive to this consumer, auto-delete when consumer disconnects)
    amqp_queue_declare_ok_t* queue_result = amqp_queue_declare(
        connection_,
        channel_,
        amqp_cstring_bytes(config_.queue_name.c_str()),  // queue name
        0,                                                // passive
        0,                                                // durable (false for exclusive queues)
        1,                                                // exclusive
        1,                                                // auto_delete
        amqp_empty_table                                  // arguments
    );
    reply = amqp_get_rpc_reply(connection_);
    if (!checkAmqpStatus(reply, "Queue declare")) {
        throw std::runtime_error("Failed to declare queue");
    }

    // Bind queue to exchange with routing keys
    for (const auto& routing_key : config_.routing_keys) {
        amqp_queue_bind(
            connection_,
            channel_,
            amqp_cstring_bytes(config_.queue_name.c_str()),
            amqp_cstring_bytes(config_.exchange.c_str()),
            amqp_cstring_bytes(routing_key.c_str()),
            amqp_empty_table
        );
        reply = amqp_get_rpc_reply(connection_);
        if (!checkAmqpStatus(reply, "Queue bind for " + routing_key)) {
            Logger::warn("Failed to bind queue to routing key: {}", routing_key);
        } else {
            Logger::info("Bound queue to routing key: {}", routing_key);
        }
    }

    // Start consuming
    amqp_basic_consume(
        connection_,
        channel_,
        amqp_cstring_bytes(config_.queue_name.c_str()),
        amqp_empty_bytes,     // consumer_tag (auto-generated)
        0,                    // no_local
        1,                    // no_ack (auto-ack messages)
        1,                    // exclusive
        amqp_empty_table
    );
    reply = amqp_get_rpc_reply(connection_);
    if (!checkAmqpStatus(reply, "Basic consume")) {
        throw std::runtime_error("Failed to start consuming");
    }

    Logger::info("Connected to RabbitMQ consumer at {}:{} vhost={} queue={}", 
                 config_.host, config_.port, config_.virtual_host, config_.queue_name);
}

void RabbitMqMessageConsumer::close() {
    if (connection_) {
        amqp_channel_close(connection_, channel_, AMQP_REPLY_SUCCESS);
        amqp_connection_close(connection_, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(connection_);
        connection_ = nullptr;
        socket_ = nullptr;
    }
}

void RabbitMqMessageConsumer::startConsuming(MessageHandler handler) {
    if (running_) {
        Logger::warn("Consumer is already running");
        return;
    }

    // Connect in the background thread
    running_ = true;
    consumer_thread_ = std::make_unique<std::thread>([this, handler]() {
        try {
            connect();
            consumeLoop(handler);
        } catch (const std::exception& e) {
            Logger::error("Consumer thread failed: {}", e.what());
            running_ = false;
        }
    });
}

void RabbitMqMessageConsumer::stopConsuming() {
    if (!running_) {
        return;
    }

    Logger::info("Stopping message consumer...");
    running_ = false;

    if (consumer_thread_ && consumer_thread_->joinable()) {
        consumer_thread_->join();
    }
    
    close();
    Logger::info("Message consumer stopped");
}

void RabbitMqMessageConsumer::consumeLoop(MessageHandler handler) {
    Logger::info("Starting message consume loop");

    while (running_) {
        amqp_envelope_t envelope;
        struct timeval timeout;
        timeout.tv_sec = 1;   // 1 second timeout for checking running_ flag
        timeout.tv_usec = 0;

        amqp_rpc_reply_t reply = amqp_consume_message(connection_, &envelope, &timeout, 0);

        if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
            // Extract routing key
            std::string routing_key(
                static_cast<const char*>(envelope.routing_key.bytes),
                envelope.routing_key.len
            );

            // Extract message body
            std::string message_body(
                static_cast<const char*>(envelope.message.body.bytes),
                envelope.message.body.len
            );

            try {
                // Parse JSON payload
                json payload = json::parse(message_body);
                
                Logger::debug("Received message on routing key: {}", routing_key);
                
                // Call handler
                handler(routing_key, payload);
            } catch (const std::exception& e) {
                Logger::error("Error processing message: {}", e.what());
            }

            amqp_destroy_envelope(&envelope);
        } else if (reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
            if (reply.library_error == AMQP_STATUS_TIMEOUT) {
                // Timeout is normal - just loop again to check running_ flag
                continue;
            } else {
                Logger::error("Consumer library error: {}", amqp_error_string2(reply.library_error));
                break;
            }
        } else {
            Logger::error("Consumer error: reply_type={}", reply.reply_type);
            break;
        }
    }

    Logger::info("Exiting consume loop");
}

bool RabbitMqMessageConsumer::checkAmqpStatus(amqp_rpc_reply_t reply, const std::string& context) {
    switch (reply.reply_type) {
        case AMQP_RESPONSE_NORMAL:
            return true;

        case AMQP_RESPONSE_NONE:
            Logger::error("{}: No response", context);
            return false;

        case AMQP_RESPONSE_LIBRARY_EXCEPTION:
            Logger::error("{}: Library exception: {}", context, amqp_error_string2(reply.library_error));
            return false;

        case AMQP_RESPONSE_SERVER_EXCEPTION:
            switch (reply.reply.id) {
                case AMQP_CONNECTION_CLOSE_METHOD: {
                    amqp_connection_close_t* m = (amqp_connection_close_t*)reply.reply.decoded;
                    Logger::error("{}: Connection closed: {}", context, 
                                std::string((char*)m->reply_text.bytes, m->reply_text.len));
                    break;
                }
                case AMQP_CHANNEL_CLOSE_METHOD: {
                    amqp_channel_close_t* m = (amqp_channel_close_t*)reply.reply.decoded;
                    Logger::error("{}: Channel closed: {}", context,
                                std::string((char*)m->reply_text.bytes, m->reply_text.len));
                    break;
                }
                default:
                    Logger::error("{}: Unknown server error, method id: {}", context, reply.reply.id);
                    break;
            }
            return false;

        default:
            Logger::error("{}: Unknown reply type: {}", context, reply.reply_type);
            return false;
    }
}

} // namespace utils
} // namespace inventory

#include "inventory/utils/RabbitMqMessageBus.hpp"
#include "inventory/utils/Logger.hpp"

#include <stdexcept>

namespace inventory {
namespace utils {

namespace {

void checkAmqpStatus(const char* context, int status) {
    if (status < 0) {
        throw std::runtime_error(std::string(context) + ": " + amqp_error_string2(status));
    }
}

void checkAmqpReply(const char* context, const amqp_rpc_reply_t& reply) {
    if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
        return;
    }
    std::string message = context;
    message += ": AMQP error";
    throw std::runtime_error(message);
}

} // namespace

RabbitMqMessageBus::RabbitMqMessageBus(const MessageBus::Config& config)
    : config_(config), connection_(nullptr), socket_(nullptr), channel_(1) {
    try {
        connect();
        utils::Logger::info("Connected to RabbitMQ at {}:{} vhost={} exchange={}",
                            config_.host, config_.port, config_.virtual_host, config_.exchange);
    } catch (const std::exception& ex) {
        utils::Logger::error("Failed to initialize RabbitMQ message bus: {}", ex.what());
        // Leave connection_ as null; publish() will no-op if not connected.
        connection_ = nullptr;
        socket_ = nullptr;
    }
}

RabbitMqMessageBus::~RabbitMqMessageBus() {
    close();
}

void RabbitMqMessageBus::connect() {
    connection_ = amqp_new_connection();
    socket_ = amqp_tcp_socket_new(connection_);
    if (!socket_) {
        throw std::runtime_error("Failed to create AMQP TCP socket");
    }

    int status = amqp_socket_open(socket_, config_.host.c_str(), config_.port);
    checkAmqpStatus("Opening TCP socket", status);

    amqp_rpc_reply_t loginReply = amqp_login(
        connection_,
        config_.virtual_host.c_str(),
        0,
        131072,
        0,
        AMQP_SASL_METHOD_PLAIN,
        config_.username.c_str(),
        config_.password.c_str());
    checkAmqpReply("Logging in to RabbitMQ", loginReply);

    amqp_channel_open_ok_t* channelOk = amqp_channel_open(connection_, channel_);
    (void)channelOk; // suppress unused warning
    checkAmqpReply("Opening channel", amqp_get_rpc_reply(connection_));

    // Declare exchange (idempotent) as topic
    amqp_exchange_declare_ok_t* exOk = amqp_exchange_declare(
        connection_,
        channel_,
        amqp_cstring_bytes(config_.exchange.c_str()),
        amqp_cstring_bytes("topic"),
        0,    // passive
        0,    // durable
        0,    // auto_delete
        0,    // internal
        amqp_empty_table);
    (void)exOk;
    checkAmqpReply("Declaring exchange", amqp_get_rpc_reply(connection_));
}

void RabbitMqMessageBus::close() {
    if (!connection_) {
        return;
    }

    try {
        amqp_channel_close(connection_, channel_, AMQP_REPLY_SUCCESS);
        amqp_connection_close(connection_, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(connection_);
    } catch (...) {
        // Suppress all exceptions during shutdown
    }

    connection_ = nullptr;
    socket_ = nullptr;
}

bool RabbitMqMessageBus::isConnected() const {
    return connection_ != nullptr;
}

void RabbitMqMessageBus::publish(const std::string& routingKey,
                                 const nlohmann::json& payload) {
    if (!connection_) {
        // Bus not available; log and return without throwing
        utils::Logger::warn("RabbitMQ message bus not connected; skipping publish for routing key {}", routingKey);
        return;
    }

    const std::string fullRoutingKey = config_.routing_key_prefix + routingKey;
    const std::string body = payload.dump();

    amqp_bytes_t messageBytes;
    messageBytes.len = body.size();
    messageBytes.bytes = const_cast<char*>(body.data());

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2; // persistent

    int status = amqp_basic_publish(
        connection_,
        channel_,
        amqp_cstring_bytes(config_.exchange.c_str()),
        amqp_cstring_bytes(fullRoutingKey.c_str()),
        0,   // mandatory
        0,   // immediate
        &props,
        messageBytes);

    if (status != AMQP_STATUS_OK) {
        utils::Logger::error("Failed to publish message to RabbitMQ (routing key {}): {}",
                             fullRoutingKey, amqp_error_string2(status));
        return;
    }

    utils::Logger::debug("Published message to RabbitMQ exchange={} routingKey={} payloadSize={} bytes",
                         config_.exchange, fullRoutingKey, body.size());
}

} // namespace utils
} // namespace inventory

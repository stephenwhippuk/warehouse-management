#pragma once

#include "inventory/utils/MessageBus.hpp"

#include <amqp.h>
#include <amqp_tcp_socket.h>

namespace inventory {
namespace utils {

class RabbitMqMessageBus : public MessageBus {
public:
    explicit RabbitMqMessageBus(const MessageBus::Config& config);
    ~RabbitMqMessageBus() override;

    void publish(const std::string& routingKey,
                 const nlohmann::json& payload) override;

    bool isConnected() const;

private:
    void connect();
    void close();

    MessageBus::Config config_;
    amqp_connection_state_t connection_;
    amqp_socket_t* socket_;
    amqp_channel_t channel_;
};

} // namespace utils
} // namespace inventory

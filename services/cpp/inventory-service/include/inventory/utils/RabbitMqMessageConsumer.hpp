#pragma once

#include "inventory/utils/MessageConsumer.hpp"
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <atomic>
#include <thread>
#include <memory>

namespace inventory {
namespace utils {

/**
 * @brief RabbitMQ implementation of MessageConsumer
 */
class RabbitMqMessageConsumer : public MessageConsumer {
public:
    explicit RabbitMqMessageConsumer(const Config& config);
    ~RabbitMqMessageConsumer() override;

    void startConsuming(MessageHandler handler) override;
    void stopConsuming() override;
    bool isRunning() const override { return running_; }

private:
    void connect();
    void close();
    void consumeLoop(MessageHandler handler);
    bool checkAmqpStatus(amqp_rpc_reply_t reply, const std::string& context);

    Config config_;
    amqp_connection_state_t connection_;
    amqp_socket_t* socket_;
    int channel_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> consumer_thread_;
};

} // namespace utils
} // namespace inventory

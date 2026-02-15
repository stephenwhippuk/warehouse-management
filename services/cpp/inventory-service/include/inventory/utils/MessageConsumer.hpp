#pragma once

#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace inventory {
namespace utils {

using json = nlohmann::json;

/**
 * @brief Abstract interface for consuming messages from a message bus
 */
class MessageConsumer {
public:
    /**
     * @brief Configuration for message consumer
     */
    struct Config {
        std::string host;
        std::string port;
        std::string virtual_host;
        std::string username;
        std::string password;
        std::string exchange;
        std::string queue_name;
        std::vector<std::string> routing_keys;  // Topics to subscribe to
    };

    /**
     * @brief Message handler callback
     * 
     * @param routingKey The routing key of the received message
     * @param payload The message payload as JSON
     */
    using MessageHandler = std::function<void(const std::string& routingKey, const json& payload)>;

    virtual ~MessageConsumer() = default;

    /**
     * @brief Start consuming messages
     * 
     * @param handler Callback function to handle incoming messages
     */
    virtual void startConsuming(MessageHandler handler) = 0;

    /**
     * @brief Stop consuming messages
     */
    virtual void stopConsuming() = 0;

    /**
     * @brief Check if consumer is running
     */
    virtual bool isRunning() const = 0;
};

} // namespace utils
} // namespace inventory

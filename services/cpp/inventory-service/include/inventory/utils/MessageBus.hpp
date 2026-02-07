#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace inventory {
namespace utils {

class MessageBus {
public:
    struct Config {
        std::string host;
        int port;
        std::string virtual_host;
        std::string username;
        std::string password;
        std::string exchange;
        std::string routing_key_prefix;
    };

    virtual ~MessageBus() = default;

    virtual void publish(const std::string& routingKey,
                         const nlohmann::json& payload) = 0;
};

} // namespace utils
} // namespace inventory

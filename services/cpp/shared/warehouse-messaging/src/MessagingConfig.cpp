#include "warehouse/messaging/MessagingConfig.hpp"
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace warehouse {
namespace messaging {

using json = nlohmann::json;

MessagingConfig MessagingConfig::fromEnvironment(const std::string& serviceName) {
    MessagingConfig config;
    
    // Read from environment variables
    if (const char* envHost = std::getenv("RABBITMQ_HOST")) {
        config.host = envHost;
    }
    
    if (const char* envPort = std::getenv("RABBITMQ_PORT")) {
        config.port = std::stoi(envPort);
    }
    
    if (const char* envVHost = std::getenv("RABBITMQ_VHOST")) {
        config.virtualHost = envVHost;
    }
    
    if (const char* envUser = std::getenv("RABBITMQ_USER")) {
        config.username = envUser;
    }
    
    if (const char* envPass = std::getenv("RABBITMQ_PASSWORD")) {
        config.password = envPass;
    }
    
    if (const char* envServiceName = std::getenv("SERVICE_NAME")) {
        config.serviceName = envServiceName;
    } else if (!serviceName.empty()) {
        config.serviceName = serviceName;
    }
    
    if (const char* envExchange = std::getenv("RABBITMQ_EXCHANGE")) {
        config.exchange = envExchange;
    }
    
    return config;
}

MessagingConfig MessagingConfig::fromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + path);
    }
    
    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        throw std::runtime_error(std::string("Failed to parse config file: ") + e.what());
    }
   
    MessagingConfig config;
    
    if (j.contains("messageBus")) {
        auto mb = j["messageBus"];
        config.host = mb.value("host", config.host);
        config.port = mb.value("port", config.port);
        config.virtualHost = mb.value("virtualHost", config.virtualHost);
        config.username = mb.value("username", config.username);
        config.password = mb.value("password", config.password);
        config.exchange = mb.value("exchange", config.exchange);
    }
    
    if (j.contains("service") && j["service"].contains("name")) {
        config.serviceName = j["service"]["name"].get<std::string>();
    }
    
    return config;
}

ConsumerConfig ConsumerConfig::withDefaults(
    const std::string& serviceName,
    const std::vector<std::string>& routingKeys
) {
    ConsumerConfig config;
    config.serviceName = serviceName;
    config.queuePrefix = serviceName;
    config.routingKeys = routingKeys;
    
    // Load connection settings from environment
    auto base = MessagingConfig::fromEnvironment(serviceName);
    config.host = base.host;
    config.port = base.port;
    config.virtualHost = base.virtualHost;
    config.username = base.username;
    config.password = base.password;
    config.exchange = base.exchange;
    
    return config;
}

std::string ConsumerConfig::getQueueName() const {
    if (queuePrefix.empty()) {
        return serviceName + "-events";
    }
    return queuePrefix + "-events";
}

PublisherConfig PublisherConfig::withDefaults(const std::string& serviceName) {
    PublisherConfig config;
    config.serviceName = serviceName;
    
    // Load connection settings from environment
    auto base = MessagingConfig::fromEnvironment(serviceName);
    config.host = base.host;
    config.port = base.port;
    config.virtualHost = base.virtualHost;
    config.username = base.username;
    config.password = base.password;
    config.exchange = base.exchange;
    
    return config;
}

} // namespace messaging
} // namespace warehouse

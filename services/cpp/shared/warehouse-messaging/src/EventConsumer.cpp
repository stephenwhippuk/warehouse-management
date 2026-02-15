#include "warehouse/messaging/EventConsumer.hpp"
#include "warehouse/messaging/internal/RabbitMqConsumer.hpp"

namespace warehouse {
namespace messaging {

std::unique_ptr<EventConsumer> EventConsumer::create(
    const std::string& serviceName,
    const std::vector<std::string>& routingKeys
) {
    auto config = ConsumerConfig::withDefaults(serviceName, routingKeys);
    return std::make_unique<internal::RabbitMqConsumer>(config);
}

std::unique_ptr<EventConsumer> EventConsumer::create(const ConsumerConfig& config) {
    return std::make_unique<internal::RabbitMqConsumer>(config);
}

} // namespace messaging
} // namespace warehouse

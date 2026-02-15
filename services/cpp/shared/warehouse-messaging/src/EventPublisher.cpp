#include "warehouse/messaging/EventPublisher.hpp"
#include "warehouse/messaging/internal/RabbitMqPublisher.hpp"

namespace warehouse {
namespace messaging {

std::unique_ptr<EventPublisher> EventPublisher::create(const std::string& serviceName) {
    auto config = PublisherConfig::withDefaults(serviceName);
    return std::make_unique<internal::RabbitMqPublisher>(config);
}

std::unique_ptr<EventPublisher> EventPublisher::create(const PublisherConfig& config) {
    return std::make_unique<internal::RabbitMqPublisher>(config);
}

} // namespace messaging
} // namespace warehouse

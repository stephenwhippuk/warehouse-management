#include <catch2/catch_all.hpp>

#include "inventory/utils/RabbitMqMessageBus.hpp"

#include <cstdlib>
#include <string>
#include <nlohmann/json.hpp>

using inventory::utils::MessageBus;
using inventory::utils::RabbitMqMessageBus;

namespace {

int getEnvInt(const char* name, int defaultValue) {
    if (const char* value = std::getenv(name)) {
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

} // namespace

TEST_CASE("RabbitMQ message bus can connect and publish", "[inventory][messagebus][integration]") {
    const char* enabled = std::getenv("INVENTORY_RABBITMQ_INTEGRATION");
    if (!enabled) {
        WARN("INVENTORY_RABBITMQ_INTEGRATION not set; skipping RabbitMQ integration tests");
        return;
    }

    MessageBus::Config config;
    config.host = std::getenv("RABBITMQ_HOST") ? std::getenv("RABBITMQ_HOST") : std::string("rabbitmq");
    config.port = getEnvInt("RABBITMQ_PORT", 5672);
    config.virtual_host = std::getenv("RABBITMQ_VHOST") ? std::getenv("RABBITMQ_VHOST") : std::string("/");
    config.username = std::getenv("RABBITMQ_USER") ? std::getenv("RABBITMQ_USER") : std::string("warehouse");
    config.password = std::getenv("RABBITMQ_PASSWORD") ? std::getenv("RABBITMQ_PASSWORD") : std::string("warehouse_dev");
    config.exchange = std::getenv("RABBITMQ_EXCHANGE") ? std::getenv("RABBITMQ_EXCHANGE") : std::string("warehouse.events");
    config.routing_key_prefix = "inventory.test.";

    RabbitMqMessageBus bus(config);

    // We expect the test environment to provide a reachable RabbitMQ; connection failures
    // would be visible through a failed assertion here.
    REQUIRE(bus.isConnected());

    nlohmann::json payload = {
        {"ping", "pong"}
    };

    REQUIRE_NOTHROW(bus.publish("ping", payload));
}

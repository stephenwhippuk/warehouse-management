#include <catch2/catch_all.hpp>

#include "inventory/services/InventoryService.hpp"

using inventory::services::InventoryService;

namespace {

class FakeMessageBus : public inventory::utils::MessageBus {
public:
    void publish(const std::string& routingKey,
                 const nlohmann::json& payload) override {
        lastRoutingKey = routingKey;
        lastPayload = payload;
    }

    std::string lastRoutingKey;
    nlohmann::json lastPayload;
};

} // namespace

TEST_CASE("InventoryService can be constructed with a MessageBus stub", "[inventory][service][messagebus]") {
    std::shared_ptr<inventory::repositories::InventoryRepository> nullRepo;
    auto fakeBus = std::make_shared<FakeMessageBus>();

    InventoryService service(nullRepo, fakeBus);

    // For now we only verify that construction succeeds with a bus dependency.
    REQUIRE(true);
}

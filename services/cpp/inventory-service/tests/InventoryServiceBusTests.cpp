#include <catch2/catch_all.hpp>

#include "inventory/services/InventoryService.hpp"
#include "inventory/repositories/InventoryRepository.hpp"
#include "inventory/utils/Database.hpp"
#include "inventory/models/Inventory.hpp"

using inventory::services::InventoryService;
using inventory::repositories::InventoryRepository;
using inventory::models::Inventory;

namespace {

class FakeMessageBus : public inventory::utils::MessageBus {
public:
    void publish(const std::string& routingKey,
                 const nlohmann::json& payload) override {
        lastRoutingKey = routingKey;
        lastPayload = payload;
        publishCount++;
    }

    std::string lastRoutingKey;
    nlohmann::json lastPayload;
    int publishCount = 0;
};

} // namespace

TEST_CASE("InventoryService can be constructed with a MessageBus stub", "[inventory][service][messagebus]") {
    std::shared_ptr<inventory::repositories::InventoryRepository> nullRepo;
    auto fakeBus = std::make_shared<FakeMessageBus>();

    InventoryService service(nullRepo, fakeBus);

    // For now we only verify that construction succeeds with a bus dependency.
    REQUIRE(true);
}

TEST_CASE("InventoryService publishes events for core operations", "[inventory][service][messagebus][db]") {
    const char* connStr = std::getenv("INVENTORY_TEST_DATABASE_URL");
    if (!connStr) {
        WARN("INVENTORY_TEST_DATABASE_URL not set; skipping InventoryService message bus DB-backed tests");
        return;
    }

    auto conn = inventory::utils::Database::connect(connStr);
    auto repo = std::make_shared<InventoryRepository>(conn);
    auto fakeBus = std::make_shared<FakeMessageBus>();

    InventoryService service(repo, fakeBus);

    const std::string id         = "77777777-7777-7777-7777-777777777777";
    const std::string productId  = "22222222-2222-2222-2222-222222222222";
    const std::string warehouseId= "33333333-3333-3333-3333-333333333333";
    const std::string locationId = "44444444-4444-4444-4444-444444444444";

    // Ensure a clean slate for this ID
    {
        pqxx::work cleanup(*conn);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", id);
        cleanup.commit();
    }

    SECTION("create publishes inventory.created event with payload including ID") {
        Inventory inv;
        inv.setId(id);
        inv.setProductId(productId);
        inv.setWarehouseId(warehouseId);
        inv.setLocationId(locationId);
        inv.setQuantity(10);
        inv.setAvailableQuantity(10);
        inv.setReservedQuantity(0);
        inv.setAllocatedQuantity(0);

        auto created = service.create(inv);

        REQUIRE(fakeBus->publishCount == 1);
        REQUIRE(fakeBus->lastRoutingKey == "created");
        REQUIRE(fakeBus->lastPayload.contains("id"));
        REQUIRE(fakeBus->lastPayload["id"].get<std::string>() == created.getId());
        REQUIRE(fakeBus->lastPayload["productId"].get<std::string>() == created.getProductId());
    }

    SECTION("update publishes inventory.updated event with new quantities") {
        // Seed via service.create (will publish a created event which we ignore)
        Inventory inv;
        inv.setId(id);
        inv.setProductId(productId);
        inv.setWarehouseId(warehouseId);
        inv.setLocationId(locationId);
        inv.setQuantity(10);
        inv.setAvailableQuantity(10);
        inv.setReservedQuantity(0);
        inv.setAllocatedQuantity(0);

        auto created = service.create(inv);

        // Reset bus state before update
        fakeBus->lastRoutingKey.clear();
        fakeBus->lastPayload = nlohmann::json();
        fakeBus->publishCount = 0;

        created.setQuantity(20);
        created.setAvailableQuantity(20);

        auto updated = service.update(created);

        REQUIRE(fakeBus->publishCount == 1);
        REQUIRE(fakeBus->lastRoutingKey == "updated");
        REQUIRE(fakeBus->lastPayload["id"].get<std::string>() == updated.getId());
        REQUIRE(fakeBus->lastPayload["quantity"].get<int>() == 20);
        REQUIRE(fakeBus->lastPayload["availableQuantity"].get<int>() == 20);
    }

    SECTION("remove publishes inventory.deleted event with id and event fields") {
        // Seed via service.create
        Inventory inv;
        inv.setId(id);
        inv.setProductId(productId);
        inv.setWarehouseId(warehouseId);
        inv.setLocationId(locationId);
        inv.setQuantity(5);
        inv.setAvailableQuantity(5);
        inv.setReservedQuantity(0);
        inv.setAllocatedQuantity(0);

        service.create(inv);

        // Reset bus state before remove
        fakeBus->lastRoutingKey.clear();
        fakeBus->lastPayload = nlohmann::json();
        fakeBus->publishCount = 0;

        bool deleted = service.remove(id);
        REQUIRE(deleted);

        REQUIRE(fakeBus->publishCount == 1);
        REQUIRE(fakeBus->lastRoutingKey == "deleted");
        REQUIRE(fakeBus->lastPayload["id"].get<std::string>() == id);
        REQUIRE(fakeBus->lastPayload["event"].get<std::string>() == "deleted");
    }

    SECTION("reserve publishes inventory.reserved event with action and quantity") {
        Inventory inv;
        inv.setId(id);
        inv.setProductId(productId);
        inv.setWarehouseId(warehouseId);
        inv.setLocationId(locationId);
        inv.setQuantity(10);
        inv.setAvailableQuantity(10);
        inv.setReservedQuantity(0);
        inv.setAllocatedQuantity(0);

        service.create(inv);

        // Reset bus state before reserve
        fakeBus->lastRoutingKey.clear();
        fakeBus->lastPayload = nlohmann::json();
        fakeBus->publishCount = 0;

        service.reserve(id, 3);

        REQUIRE(fakeBus->publishCount == 1);
        REQUIRE(fakeBus->lastRoutingKey == "reserved");
        REQUIRE(fakeBus->lastPayload["id"].get<std::string>() == id);
        REQUIRE(fakeBus->lastPayload["action"].get<std::string>() == "reserve");
        REQUIRE(fakeBus->lastPayload["quantity"].get<int>() == 3);
        REQUIRE(fakeBus->lastPayload["reservedQuantity"].get<int>() == 3);
        REQUIRE(fakeBus->lastPayload["availableQuantity"].get<int>() == 7);
    }

    // Clean up test data for this ID
    {
        pqxx::work cleanup(*conn);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", id);
        cleanup.commit();
    }
}

#include "inventory/repositories/InventoryRepository.hpp"

#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <regex>
#include <stdexcept>

namespace inventory {
namespace repositories {

namespace {

using json = nlohmann::json;

bool isValidUuid(const std::string& id) {
    static const std::regex uuid_regex(
        R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)"
    );
    return std::regex_match(id, uuid_regex);
}

json inventoryRowToJson(const pqxx::row& row) {
    json j;

    j["id"] = row["id"].as<std::string>();
    j["productId"] = row["product_id"].as<std::string>();
    j["warehouseId"] = row["warehouse_id"].as<std::string>();
    j["locationId"] = row["location_id"].as<std::string>();
    j["quantity"] = row["quantity"].as<int>();
    j["availableQuantity"] = row["available_quantity"].as<int>();
    j["reservedQuantity"] = row["reserved_quantity"].as<int>();
    j["allocatedQuantity"] = row["allocated_quantity"].as<int>();
    j["status"] = row["status"].as<std::string>();
    j["qualityStatus"] = row["quality_status"].as<std::string>();

    if (!row["serial_number"].is_null()) {
        j["serialNumber"] = row["serial_number"].as<std::string>();
    }
    if (!row["batch_number"].is_null()) {
        j["batchNumber"] = row["batch_number"].as<std::string>();
    }
    if (!row["expiration_date"].is_null()) {
        j["expirationDate"] = row["expiration_date"].as<std::string>();
    }
    if (!row["manufacture_date"].is_null()) {
        j["manufactureDate"] = row["manufacture_date"].as<std::string>();
    }
    if (!row["received_date"].is_null()) {
        j["receivedDate"] = row["received_date"].as<std::string>();
    }
    if (!row["last_counted_date"].is_null()) {
        j["lastCountedDate"] = row["last_counted_date"].as<std::string>();
    }
    if (!row["last_counted_by"].is_null()) {
        j["lastCountedBy"] = row["last_counted_by"].as<std::string>();
    }
    if (!row["cost_per_unit"].is_null()) {
        j["costPerUnit"] = row["cost_per_unit"].as<double>();
    }
    if (!row["notes"].is_null()) {
        j["notes"] = row["notes"].as<std::string>();
    }
    if (!row["metadata"].is_null()) {
        auto metadata_text = row["metadata"].as<std::string>();
        if (!metadata_text.empty()) {
            j["metadata"] = nlohmann::json::parse(metadata_text);
        }
    }

    json audit;
    if (!row["created_at"].is_null()) {
        audit["createdAt"] = row["created_at"].as<std::string>();
    }
    if (!row["updated_at"].is_null()) {
        audit["updatedAt"] = row["updated_at"].as<std::string>();
    }
    if (!row["created_by"].is_null()) {
        audit["createdBy"] = row["created_by"].as<std::string>();
    }
    if (!row["updated_by"].is_null()) {
        audit["updatedBy"] = row["updated_by"].as<std::string>();
    }
    if (!audit.empty()) {
        j["audit"] = audit;
    }

    return j;
}

} // namespace

InventoryRepository::InventoryRepository(std::shared_ptr<pqxx::connection> db)
    : db_(db) {}

std::optional<models::Inventory> InventoryRepository::findById(const std::string& id) {
    if (!isValidUuid(id)) {
        throw std::invalid_argument("Invalid inventory id format");
    }

    pqxx::work txn(*db_);
    auto result = txn.exec_params(
        "SELECT * FROM inventory WHERE id = $1",
        id
    );
    txn.commit();

    if (result.empty()) {
        return std::nullopt;
    }

    const auto& row = result[0];
    auto json_row = inventoryRowToJson(row);
    return models::Inventory::fromJson(json_row);
}

std::vector<models::Inventory> InventoryRepository::findAll() {
    pqxx::work txn(*db_);
    auto result = txn.exec(
        "SELECT * FROM inventory ORDER BY created_at DESC"
    );
    txn.commit();

    std::vector<models::Inventory> inventories;
    inventories.reserve(result.size());

    for (const auto& row : result) {
        auto json_row = inventoryRowToJson(row);
        inventories.push_back(models::Inventory::fromJson(json_row));
    }

    return inventories;
}

std::vector<models::Inventory> InventoryRepository::findByProductId(const std::string& productId) {
    if (!isValidUuid(productId)) {
        throw std::invalid_argument("Invalid product id format");
    }

    pqxx::work txn(*db_);
    auto result = txn.exec_params(
        "SELECT * FROM inventory WHERE product_id = $1 ORDER BY created_at DESC",
        productId
    );
    txn.commit();

    std::vector<models::Inventory> inventories;
    inventories.reserve(result.size());

    for (const auto& row : result) {
        auto json_row = inventoryRowToJson(row);
        inventories.push_back(models::Inventory::fromJson(json_row));
    }

    return inventories;
}

std::vector<models::Inventory> InventoryRepository::findByWarehouseId(const std::string& warehouseId) {
    if (!isValidUuid(warehouseId)) {
        throw std::invalid_argument("Invalid warehouse id format");
    }

    pqxx::work txn(*db_);
    auto result = txn.exec_params(
        "SELECT * FROM inventory WHERE warehouse_id = $1 ORDER BY created_at DESC",
        warehouseId
    );
    txn.commit();

    std::vector<models::Inventory> inventories;
    inventories.reserve(result.size());

    for (const auto& row : result) {
        auto json_row = inventoryRowToJson(row);
        inventories.push_back(models::Inventory::fromJson(json_row));
    }

    return inventories;
}

std::vector<models::Inventory> InventoryRepository::findByLocationId(const std::string& locationId) {
    if (!isValidUuid(locationId)) {
        throw std::invalid_argument("Invalid location id format");
    }

    pqxx::work txn(*db_);
    auto result = txn.exec_params(
        "SELECT * FROM inventory WHERE location_id = $1 ORDER BY created_at DESC",
        locationId
    );
    txn.commit();

    std::vector<models::Inventory> inventories;
    inventories.reserve(result.size());

    for (const auto& row : result) {
        auto json_row = inventoryRowToJson(row);
        inventories.push_back(models::Inventory::fromJson(json_row));
    }

    return inventories;
}

std::vector<models::Inventory> InventoryRepository::findLowStock(int threshold) {
    if (threshold < 0) {
        throw std::invalid_argument("Threshold must be non-negative");
    }

    pqxx::work txn(*db_);
    auto result = txn.exec_params(
        "SELECT * FROM inventory WHERE available_quantity < $1 ORDER BY available_quantity ASC",
        threshold
    );
    txn.commit();

    std::vector<models::Inventory> inventories;
    inventories.reserve(result.size());

    for (const auto& row : result) {
        auto json_row = inventoryRowToJson(row);
        inventories.push_back(models::Inventory::fromJson(json_row));
    }

    return inventories;
}

std::vector<models::Inventory> InventoryRepository::findExpired() {
    pqxx::work txn(*db_);
    auto result = txn.exec(
        "SELECT * FROM inventory WHERE expiration_date < CURRENT_DATE AND expiration_date IS NOT NULL ORDER BY expiration_date ASC"
    );
    txn.commit();

    std::vector<models::Inventory> inventories;
    inventories.reserve(result.size());

    for (const auto& row : result) {
        auto json_row = inventoryRowToJson(row);
        inventories.push_back(models::Inventory::fromJson(json_row));
    }

    return inventories;
}

std::optional<models::Inventory> InventoryRepository::findByProductAndLocation(
    const std::string& productId, 
    const std::string& locationId) {
    if (!isValidUuid(productId)) {
        throw std::invalid_argument("Invalid product id format");
    }
    if (!isValidUuid(locationId)) {
        throw std::invalid_argument("Invalid location id format");
    }

    pqxx::work txn(*db_);
    auto result = txn.exec_params(
        "SELECT * FROM inventory WHERE product_id = $1 AND location_id = $2 LIMIT 1",
        productId,
        locationId
    );
    txn.commit();

    if (result.empty()) {
        return std::nullopt;
    }

    const auto& row = result[0];
    auto json_row = inventoryRowToJson(row);
    return models::Inventory::fromJson(json_row);
}

models::Inventory InventoryRepository::create(const models::Inventory& inventory) {
    // TODO: Implement PostgreSQL INSERT
    // INSERT INTO inventory (...) VALUES (...) RETURNING *
    return inventory;
}

models::Inventory InventoryRepository::update(const models::Inventory& inventory) {
    // TODO: Implement PostgreSQL UPDATE
    // UPDATE inventory SET ... WHERE id = $1 RETURNING *
    return inventory;
}

bool InventoryRepository::deleteById(const std::string& id) {
    // TODO: Implement PostgreSQL DELETE
    // DELETE FROM inventory WHERE id = $1
    return false;
}

int InventoryRepository::getTotalQuantityByProduct(const std::string& productId) {
    // TODO: Implement PostgreSQL query
    // SELECT SUM(quantity) FROM inventory WHERE product_id = $1
    return 0;
}

int InventoryRepository::getAvailableQuantityByProduct(const std::string& productId) {
    // TODO: Implement PostgreSQL query
    // SELECT SUM(available_quantity) FROM inventory WHERE product_id = $1
    return 0;
}

} // namespace repositories
} // namespace inventory

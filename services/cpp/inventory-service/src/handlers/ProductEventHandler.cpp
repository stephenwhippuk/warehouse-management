#include "inventory/handlers/ProductEventHandler.hpp"
#include "inventory/utils/Logger.hpp"
#include <stdexcept>

namespace inventory {
namespace handlers {

ProductEventHandler::ProductEventHandler(std::shared_ptr<pqxx::connection> db)
    : db_(db) {
    if (!db_) {
        throw std::invalid_argument("Database connection cannot be null");
    }
}

void ProductEventHandler::handleProductCreated(const json& event) {
    try {
        // Extract product data from event
        const auto& data = event["data"];
        std::string product_id = data["id"].get<std::string>();
        std::string sku = data["sku"].get<std::string>();
        std::string name = data["name"].get<std::string>();

        utils::Logger::info("Processing ProductCreated event for product {} ({})", product_id, sku);

        // Upsert into product_cache
        upsertProductCache(product_id, sku, name);

        utils::Logger::info("Successfully processed ProductCreated event for {}", product_id);
    } catch (const std::exception& e) {
        utils::Logger::error("Error handling ProductCreated event: {}", e.what());
        throw;
    }
}

void ProductEventHandler::handleProductUpdated(const json& event) {
    try {
        // Extract product data from event
        const auto& data = event["data"];
        std::string product_id = data["id"].get<std::string>();
        std::string sku = data["sku"].get<std::string>();
        std::string name = data["name"].get<std::string>();

        utils::Logger::info("Processing ProductUpdated event for product {} ({})", product_id, sku);

        // Upsert into product_cache
        upsertProductCache(product_id, sku, name);

        utils::Logger::info("Successfully processed ProductUpdated event for {}", product_id);
    } catch (const std::exception& e) {
        utils::Logger::error("Error handling ProductUpdated event: {}", e.what());
        throw;
    }
}

void ProductEventHandler::handleProductDeleted(const json& event) {
    try {
        // Extract product ID from event
        const auto& data = event["data"];
        std::string product_id = data["id"].get<std::string>();
        std::string sku = data["sku"].get<std::string>();

        utils::Logger::info("Processing ProductDeleted event for product {} ({})", product_id, sku);

        // Delete from product_cache
        deleteProductCache(product_id);

        utils::Logger::info("Successfully processed ProductDeleted event for {}", product_id);
    } catch (const std::exception& e) {
        utils::Logger::error("Error handling ProductDeleted event: {}", e.what());
        throw;
    }
}

void ProductEventHandler::upsertProductCache(const std::string& product_id, 
                                            const std::string& sku, 
                                            const std::string& name) {
    pqxx::work txn(*db_);
    
    try {
        // Use INSERT ... ON CONFLICT UPDATE (PostgreSQL UPSERT)
        txn.exec_params(
            "INSERT INTO product_cache (product_id, sku, name, cached_at, updated_at) "
            "VALUES ($1, $2, $3, NOW(), NOW()) "
            "ON CONFLICT (product_id) DO UPDATE "
            "SET sku = EXCLUDED.sku, "
            "    name = EXCLUDED.name, "
            "    updated_at = NOW()",
            product_id,
            sku,
            name
        );
        
        txn.commit();
        utils::Logger::debug("Upserted product_cache: {} - {} - {}", product_id, sku, name);
    } catch (const std::exception& e) {
        utils::Logger::error("Database error upserting product_cache: {}", e.what());
        throw;
    }
}

void ProductEventHandler::deleteProductCache(const std::string& product_id) {
    pqxx::work txn(*db_);
    
    try {
        auto result = txn.exec_params(
            "DELETE FROM product_cache WHERE product_id = $1",
            product_id
        );
        
        txn.commit();
        
        if (result.affected_rows() > 0) {
            utils::Logger::debug("Deleted product_cache entry: {}", product_id);
        } else {
            utils::Logger::warn("Product not found in cache: {}", product_id);
        }
    } catch (const std::exception& e) {
        utils::Logger::error("Database error deleting from product_cache: {}", e.what());
        throw;
    }
}

} // namespace handlers
} // namespace inventory

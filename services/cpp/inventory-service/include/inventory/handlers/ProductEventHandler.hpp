#pragma once

#include <nlohmann/json.hpp>
#include <memory>
#include <pqxx/pqxx>

namespace inventory {
namespace handlers {

using json = nlohmann::json;

/**
 * @brief Handles Product domain events and updates local product_cache
 */
class ProductEventHandler {
public:
    explicit ProductEventHandler(std::shared_ptr<pqxx::connection> db);

    /**
     * @brief Handle ProductCreated event
     */
    void handleProductCreated(const json& event);

    /**
     * @brief Handle ProductUpdated event
     */
    void handleProductUpdated(const json& event);

    /**
     * @brief Handle ProductDeleted event
     */
    void handleProductDeleted(const json& event);

private:
    std::shared_ptr<pqxx::connection> db_;

    void upsertProductCache(const std::string& product_id, 
                           const std::string& sku, 
                           const std::string& name);
    
    void deleteProductCache(const std::string& product_id);
};

} // namespace handlers
} // namespace inventory

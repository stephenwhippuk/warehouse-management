#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace product::models {

/**
 * @brief Product/SKU master data entity
 * 
 * Manages product information including SKU, name, description, category
 * and operational status. Products don't have quantity tracking - that's
 * handled by Inventory in the inventory-service.
 */
class Product {
public:
    enum class Status {
        ACTIVE,
        INACTIVE,
        DISCONTINUED
    };

    // Constructors
    Product() = default;
    
    Product(const std::string& id,
            const std::string& sku,
            const std::string& name,
            const std::optional<std::string>& description,
            const std::optional<std::string>& category,
            Status status);

    // Getters (const methods)
    std::string getId() const { return id_; }
    std::string getSku() const { return sku_; }
    std::string getName() const { return name_; }
    std::optional<std::string> getDescription() const { return description_; }
    std::optional<std::string> getCategory() const { return category_; }
    Status getStatus() const { return status_; }

    // Setters
    void setName(const std::string& name) { name_ = name; }
    void setDescription(const std::optional<std::string>& description) { description_ = description; }
    void setCategory(const std::optional<std::string>& category) { category_ = category; }
    void setStatus(Status status) { status_ = status; }

    // Serialization
    json toJson() const;
    static Product fromJson(const json& j);

private:
    std::string id_;
    std::string sku_;
    std::string name_;
    std::optional<std::string> description_;
    std::optional<std::string> category_;
    Status status_;
};

}  // namespace product::models

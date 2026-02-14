#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace product::dtos {

/**
 * @brief Product data transfer object
 * 
 * Immutable DTO for exposing product data via API.
 * All fields are validated at construction time.
 */
class ProductItemDto {
public:
    /**
     * @brief Construct DTO with all required fields - validates on construction
     */
    ProductItemDto(const std::string& id,
                   const std::string& sku,
                   const std::string& name,
                   const std::optional<std::string>& description,
                   const std::optional<std::string>& category,
                   const std::string& status);

    // Immutable getters (const, no setters)
    std::string getId() const { return id_; }
    std::string getSku() const { return sku_; }
    std::string getName() const { return name_; }
    std::optional<std::string> getDescription() const { return description_; }
    std::optional<std::string> getCategory() const { return category_; }
    std::string getStatus() const { return status_; }
    
    // Serialization
    json toJson() const;

private:
    std::string id_;
    std::string sku_;
    std::string name_;
    std::optional<std::string> description_;
    std::optional<std::string> category_;
    std::string status_;

    // Validation (called from constructor)
    void validateUuid(const std::string& uuid, const std::string& fieldName) const;
    void validateSku(const std::string& sku) const;
    void validateStatus(const std::string& status) const;
};

}  // namespace product::dtos

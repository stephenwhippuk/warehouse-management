#include "product/dtos/ProductItemDto.hpp"
#include <regex>
#include <stdexcept>

namespace product::dtos {

ProductItemDto::ProductItemDto(const std::string& id,
                               const std::string& sku,
                               const std::string& name,
                               const std::optional<std::string>& description,
                               const std::optional<std::string>& category,
                               const std::string& status)
    : id_(id)
    , sku_(sku)
    , name_(name)
    , description_(description)
    , category_(category)
    , status_(status) {
    
    // Validate all fields
    validateUuid(id_, "id");
    validateSku(sku_);
    
    if (name_.empty()) {
        throw std::invalid_argument("name cannot be empty");
    }
    if (name_.length() > 200) {
        throw std::invalid_argument("name must be at most 200 characters");
    }
    
    if (description_ && description_.value().length() > 2000) {
        throw std::invalid_argument("description must be at most 2000 characters");
    }
    
    if (category_ && category_.value().length() > 100) {
        throw std::invalid_argument("category must be at most 100 characters");
    }
    
    validateStatus(status_);
}

void ProductItemDto::validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}

void ProductItemDto::validateSku(const std::string& sku) const {
    if (sku.empty()) {
        throw std::invalid_argument("sku cannot be empty");
    }
    if (sku.length() > 100) {
        throw std::invalid_argument("sku must be at most 100 characters");
    }
    static const std::regex skuRegex("^[A-Z0-9-]+$");
    if (!std::regex_match(sku, skuRegex)) {
        throw std::invalid_argument("sku must contain only uppercase letters, digits, and hyphens");
    }
}

void ProductItemDto::validateStatus(const std::string& status) const {
    if (status != "active" && status != "inactive" && status != "discontinued") {
        throw std::invalid_argument("status must be one of: active, inactive, discontinued");
    }
}

json ProductItemDto::toJson() const {
    json j = {
        {"id", id_},
        {"sku", sku_},
        {"name", name_},
        {"status", status_}
    };
    
    if (description_) {
        j["description"] = *description_;
    }
    if (category_) {
        j["category"] = *category_;
    }
    
    return j;
}

}  // namespace product::dtos

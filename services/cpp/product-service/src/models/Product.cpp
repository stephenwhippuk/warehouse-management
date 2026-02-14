#include "product/models/Product.hpp"
#include <stdexcept>

namespace product::models {

Product::Product(const std::string& id,
                 const std::string& sku,
                 const std::string& name,
                 const std::optional<std::string>& description,
                 const std::optional<std::string>& category,
                 Status status)
    : id_(id)
    , sku_(sku)
    , name_(name)
    , description_(description)
    , category_(category)
    , status_(status) {
    
    if (id_.empty()) {
        throw std::invalid_argument("Product id cannot be empty");
    }
    if (sku_.empty()) {
        throw std::invalid_argument("Product SKU cannot be empty");
    }
    if (name_.empty()) {
        throw std::invalid_argument("Product name cannot be empty");
    }
}

json Product::toJson() const {
    json j = {
        {"id", id_},
        {"sku", sku_},
        {"name", name_}
    };
    
    if (description_) {
        j["description"] = *description_;
    }
    if (category_) {
        j["category"] = *category_;
    }
    
    // Convert status enum to string
    switch (status_) {
        case Status::ACTIVE:
            j["status"] = "active";
            break;
        case Status::INACTIVE:
            j["status"] = "inactive";
            break;
        case Status::DISCONTINUED:
            j["status"] = "discontinued";
            break;
    }
    
    return j;
}

Product Product::fromJson(const json& j) {
    std::string id = j.at("id").get<std::string>();
    std::string sku = j.at("sku").get<std::string>();
    std::string name = j.at("name").get<std::string>();
    std::optional<std::string> description = std::nullopt;
    std::optional<std::string> category = std::nullopt;
    
    if (j.contains("description") && !j["description"].is_null()) {
        description = j["description"].get<std::string>();
    }
    if (j.contains("category") && !j["category"].is_null()) {
        category = j["category"].get<std::string>();
    }
    
    // Convert status string to enum
    std::string statusStr = j.at("status").get<std::string>();
    Status status;
    if (statusStr == "active") {
        status = Status::ACTIVE;
    } else if (statusStr == "inactive") {
        status = Status::INACTIVE;
    } else if (statusStr == "discontinued") {
        status = Status::DISCONTINUED;
    } else {
        throw std::invalid_argument("Invalid product status: " + statusStr);
    }
    
    return Product(id, sku, name, description, category, status);
}

}  // namespace product::models

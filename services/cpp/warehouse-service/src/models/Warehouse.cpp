#include "warehouse/models/Warehouse.hpp"
#include <stdexcept>

namespace warehouse::models {

json Warehouse::toJson() const {
    json j;
    j["id"] = id_;
    j["code"] = code_;
    j["name"] = name_;
    
    if (description_) j["description"] = *description_;
    
    j["address"] = json(address_);
    
    if (coordinates_) {
        j["coordinates"] = json(*coordinates_);
    }
    
    j["type"] = warehouseTypeToString(type_);
    
    if (totalArea_) j["totalArea"] = *totalArea_;
    if (storageCapacity_) j["storageCapacity"] = *storageCapacity_;
    if (contactPerson_) j["contactPerson"] = json(*contactPerson_);
    if (operatingHours_) j["operatingHours"] = json(*operatingHours_);
    
    if (!capabilities_.empty()) {
        json caps = json::array();
        for (const auto& cap : capabilities_) {
            caps.push_back(warehouseCapabilityToString(cap));
        }
        j["capabilities"] = caps;
    }
    
    j["status"] = statusToString(status_);
    
    if (metadata_) j["metadata"] = json(*metadata_);
    
    json auditJson;
    to_json(auditJson, audit_);
    j["audit"] = auditJson;
    
    return j;
}

Warehouse Warehouse::fromJson(const json& j) {
    Warehouse w;
    w.id_ = j.at("id").get<std::string>();
    w.code_ = j.at("code").get<std::string>();
    w.name_ = j.at("name").get<std::string>();
    
    if (j.contains("description")) {
        w.description_ = j["description"].get<std::string>();
    }
    
    w.address_ = j.at("address").get<Address>();
    
    if (j.contains("coordinates")) {
        w.coordinates_ = j["coordinates"].get<Coordinates>();
    }
    
    w.type_ = stringToWarehouseType(j.at("type").get<std::string>());
    
    if (j.contains("totalArea")) w.totalArea_ = j["totalArea"].get<double>();
    if (j.contains("storageCapacity")) w.storageCapacity_ = j["storageCapacity"].get<double>();
    if (j.contains("contactPerson")) w.contactPerson_ = j["contactPerson"].get<ContactPerson>();
    if (j.contains("operatingHours")) w.operatingHours_ = j["operatingHours"].get<OperatingHours>();
    
    if (j.contains("capabilities")) {
        for (const auto& cap : j["capabilities"]) {
            w.capabilities_.push_back(stringToWarehouseCapability(cap.get<std::string>()));
        }
    }
    
    w.status_ = stringToStatus(j.at("status").get<std::string>());
    
    if (j.contains("metadata")) {
        w.metadata_ = j["metadata"].get<std::map<std::string, json>>();
    }
    
    from_json(j.at("audit"), w.audit_);
    
    return w;
}

std::string warehouseTypeToString(WarehouseType type) {
    switch (type) {
        case WarehouseType::Distribution: return "distribution";
        case WarehouseType::Fulfillment: return "fulfillment";
        case WarehouseType::Storage: return "storage";
        case WarehouseType::ColdStorage: return "cold_storage";
        case WarehouseType::CrossDock: return "cross_dock";
    }
    throw std::invalid_argument("Invalid WarehouseType");
}

WarehouseType stringToWarehouseType(const std::string& str) {
    if (str == "distribution") return WarehouseType::Distribution;
    if (str == "fulfillment") return WarehouseType::Fulfillment;
    if (str == "storage") return WarehouseType::Storage;
    if (str == "cold_storage") return WarehouseType::ColdStorage;
    if (str == "cross_dock") return WarehouseType::CrossDock;
    throw std::invalid_argument("Invalid warehouse type string: " + str);
}

std::string warehouseCapabilityToString(WarehouseCapability cap) {
    switch (cap) {
        case WarehouseCapability::Refrigeration: return "refrigeration";
        case WarehouseCapability::Freezer: return "freezer";
        case WarehouseCapability::Hazmat: return "hazmat";
        case WarehouseCapability::ClimateControlled: return "climate_controlled";
        case WarehouseCapability::HighBay: return "high_bay";
        case WarehouseCapability::DockDoors: return "dock_doors";
        case WarehouseCapability::RailAccess: return "rail_access";
        case WarehouseCapability::CrossDocking: return "cross_docking";
    }
    throw std::invalid_argument("Invalid WarehouseCapability");
}

WarehouseCapability stringToWarehouseCapability(const std::string& str) {
    if (str == "refrigeration") return WarehouseCapability::Refrigeration;
    if (str == "freezer") return WarehouseCapability::Freezer;
    if (str == "hazmat") return WarehouseCapability::Hazmat;
    if (str == "climate_controlled") return WarehouseCapability::ClimateControlled;
    if (str == "high_bay") return WarehouseCapability::HighBay;
    if (str == "dock_doors") return WarehouseCapability::DockDoors;
    if (str == "rail_access") return WarehouseCapability::RailAccess;
    if (str == "cross_docking") return WarehouseCapability::CrossDocking;
    throw std::invalid_argument("Invalid warehouse capability string: " + str);
}

} // namespace warehouse::models

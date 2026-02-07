#include "warehouse/models/Location.hpp"
#include <stdexcept>

namespace warehouse::models {

json Location::toJson() const {
    json j;
    j["id"] = id_;
    j["warehouseId"] = warehouseId_;
    j["code"] = code_;
    
    if (name_) j["name"] = *name_;
    
    j["type"] = locationTypeToString(type_);
    
    if (zone_) j["zone"] = *zone_;
    if (aisle_) j["aisle"] = *aisle_;
    if (rack_) j["rack"] = *rack_;
    if (shelf_) j["shelf"] = *shelf_;
    if (bin_) j["bin"] = *bin_;
    if (parentLocationId_) j["parentLocationId"] = *parentLocationId_;
    if (dimensions_) j["dimensions"] = json(*dimensions_);
    if (maxWeight_) j["maxWeight"] = json(*maxWeight_);
    if (maxVolume_) j["maxVolume"] = *maxVolume_;
    
    j["isPickable"] = isPickable_;
    j["isReceivable"] = isReceivable_;
    j["requiresEquipment"] = requiredEquipmentToString(requiresEquipment_);
    j["temperatureControlled"] = temperatureControlled_;
    
    if (temperatureRange_) j["temperatureRange"] = json(*temperatureRange_);
    if (barcode_) j["barcode"] = *barcode_;
    
    j["status"] = locationStatusToString(status_);
    
    if (metadata_) j["metadata"] = json(*metadata_);
    
    json auditJson;
    to_json(auditJson, audit_);
    j["audit"] = auditJson;
    
    return j;
}

Location Location::fromJson(const json& j) {
    Location loc;
    loc.id_ = j.at("id").get<std::string>();
    loc.warehouseId_ = j.at("warehouseId").get<std::string>();
    loc.code_ = j.at("code").get<std::string>();
    
    if (j.contains("name")) loc.name_ = j["name"].get<std::string>();
    
    loc.type_ = stringToLocationType(j.at("type").get<std::string>());
    
    if (j.contains("zone")) loc.zone_ = j["zone"].get<std::string>();
    if (j.contains("aisle")) loc.aisle_ = j["aisle"].get<std::string>();
    if (j.contains("rack")) loc.rack_ = j["rack"].get<std::string>();
    if (j.contains("shelf")) loc.shelf_ = j["shelf"].get<std::string>();
    if (j.contains("bin")) loc.bin_ = j["bin"].get<std::string>();
    if (j.contains("parentLocationId")) loc.parentLocationId_ = j["parentLocationId"].get<std::string>();
    if (j.contains("dimensions")) loc.dimensions_ = j["dimensions"].get<Dimensions>();
    if (j.contains("maxWeight")) loc.maxWeight_ = j["maxWeight"].get<Weight>();
    if (j.contains("maxVolume")) loc.maxVolume_ = j["maxVolume"].get<double>();
    
    if (j.contains("isPickable")) loc.isPickable_ = j["isPickable"].get<bool>();
    if (j.contains("isReceivable")) loc.isReceivable_ = j["isReceivable"].get<bool>();
    if (j.contains("requiresEquipment")) {
        loc.requiresEquipment_ = stringToRequiredEquipment(j["requiresEquipment"].get<std::string>());
    }
    if (j.contains("temperatureControlled")) {
        loc.temperatureControlled_ = j["temperatureControlled"].get<bool>();
    }
    if (j.contains("temperatureRange")) {
        loc.temperatureRange_ = j["temperatureRange"].get<TemperatureRange>();
    }
    if (j.contains("barcode")) loc.barcode_ = j["barcode"].get<std::string>();
    
    loc.status_ = stringToLocationStatus(j.at("status").get<std::string>());
    
    if (j.contains("metadata")) {
        loc.metadata_ = j["metadata"].get<std::map<std::string, json>>();
    }
    
    from_json(j.at("audit"), loc.audit_);
    
    return loc;
}

std::string locationTypeToString(LocationType type) {
    switch (type) {
        case LocationType::Bin: return "bin";
        case LocationType::Shelf: return "shelf";
        case LocationType::Rack: return "rack";
        case LocationType::Pallet: return "pallet";
        case LocationType::Floor: return "floor";
        case LocationType::Staging: return "staging";
        case LocationType::Receiving: return "receiving";
        case LocationType::Shipping: return "shipping";
        case LocationType::Picking: return "picking";
        case LocationType::Returns: return "returns";
    }
    throw std::invalid_argument("Invalid LocationType");
}

LocationType stringToLocationType(const std::string& str) {
    if (str == "bin") return LocationType::Bin;
    if (str == "shelf") return LocationType::Shelf;
    if (str == "rack") return LocationType::Rack;
    if (str == "pallet") return LocationType::Pallet;
    if (str == "floor") return LocationType::Floor;
    if (str == "staging") return LocationType::Staging;
    if (str == "receiving") return LocationType::Receiving;
    if (str == "shipping") return LocationType::Shipping;
    if (str == "picking") return LocationType::Picking;
    if (str == "returns") return LocationType::Returns;
    throw std::invalid_argument("Invalid location type string: " + str);
}

std::string locationStatusToString(LocationStatus status) {
    switch (status) {
        case LocationStatus::Active: return "active";
        case LocationStatus::Inactive: return "inactive";
        case LocationStatus::Full: return "full";
        case LocationStatus::Reserved: return "reserved";
        case LocationStatus::Damaged: return "damaged";
        case LocationStatus::Maintenance: return "maintenance";
    }
    throw std::invalid_argument("Invalid LocationStatus");
}

LocationStatus stringToLocationStatus(const std::string& str) {
    if (str == "active") return LocationStatus::Active;
    if (str == "inactive") return LocationStatus::Inactive;
    if (str == "full") return LocationStatus::Full;
    if (str == "reserved") return LocationStatus::Reserved;
    if (str == "damaged") return LocationStatus::Damaged;
    if (str == "maintenance") return LocationStatus::Maintenance;
    throw std::invalid_argument("Invalid location status string: " + str);
}

std::string requiredEquipmentToString(RequiredEquipment equipment) {
    switch (equipment) {
        case RequiredEquipment::None: return "none";
        case RequiredEquipment::Forklift: return "forklift";
        case RequiredEquipment::Ladder: return "ladder";
        case RequiredEquipment::CherryPicker: return "cherry_picker";
        case RequiredEquipment::PalletJack: return "pallet_jack";
    }
    throw std::invalid_argument("Invalid RequiredEquipment");
}

RequiredEquipment stringToRequiredEquipment(const std::string& str) {
    if (str == "none") return RequiredEquipment::None;
    if (str == "forklift") return RequiredEquipment::Forklift;
    if (str == "ladder") return RequiredEquipment::Ladder;
    if (str == "cherry_picker") return RequiredEquipment::CherryPicker;
    if (str == "pallet_jack") return RequiredEquipment::PalletJack;
    throw std::invalid_argument("Invalid required equipment string: " + str);
}

} // namespace warehouse::models

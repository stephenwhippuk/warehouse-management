#include "warehouse/models/Common.hpp"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace warehouse::models {

void to_json(json& j, const timestamp& t) {
    auto time = std::chrono::system_clock::to_time_t(t);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    j = ss.str();
}

void from_json(const json& j, timestamp& t) {
    std::string timeStr = j.get<std::string>();
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    t = std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

void to_json(json& j, const AuditInfo& audit) {
    json createdAt;
    to_json(createdAt, audit.createdAt);
    j["createdAt"] = createdAt;
    j["createdBy"] = audit.createdBy;
    
    if (audit.updatedAt) {
        json updatedAt;
        to_json(updatedAt, *audit.updatedAt);
        j["updatedAt"] = updatedAt;
    }
    
    if (audit.updatedBy) {
        j["updatedBy"] = *audit.updatedBy;
    }
}

void from_json(const json& j, AuditInfo& audit) {
    from_json(j.at("createdAt"), audit.createdAt);
    audit.createdBy = j.at("createdBy").get<std::string>();
    
    if (j.contains("updatedAt")) {
        timestamp updatedAt;
        from_json(j["updatedAt"], updatedAt);
        audit.updatedAt = updatedAt;
    }
    
    if (j.contains("updatedBy")) {
        audit.updatedBy = j["updatedBy"].get<std::string>();
    }
}

std::string statusToString(Status status) {
    switch (status) {
        case Status::Active: return "active";
        case Status::Inactive: return "inactive";
        case Status::Archived: return "archived";
    }
    throw std::invalid_argument("Invalid Status");
}

Status stringToStatus(const std::string& str) {
    if (str == "active") return Status::Active;
    if (str == "inactive") return Status::Inactive;
    if (str == "archived") return Status::Archived;
    throw std::invalid_argument("Invalid status string: " + str);
}

} // namespace warehouse::models

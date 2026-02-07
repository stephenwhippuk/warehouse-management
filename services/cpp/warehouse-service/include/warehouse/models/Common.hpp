#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace warehouse::models {

using json = nlohmann::json;
using timestamp = std::chrono::system_clock::time_point;

/**
 * @brief Common types used across all models (matching common.schema.json)
 */

struct Address {
    std::string street;
    std::optional<std::string> street2;
    std::string city;
    std::optional<std::string> state;
    std::string postalCode;
    std::string country; // ISO 3166-1 alpha-2

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Address, street, street2, city, state, postalCode, country)
};

struct Coordinates {
    double latitude;
    double longitude;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Coordinates, latitude, longitude)
};

struct Dimensions {
    double length;
    double width;
    double height;
    std::string unit; // mm, cm, m, in, ft

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Dimensions, length, width, height, unit)
};

struct Weight {
    double value;
    std::string unit; // g, kg, oz, lb

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Weight, value, unit)
};

struct AuditInfo {
    timestamp createdAt;
    std::string createdBy;
    std::optional<timestamp> updatedAt;
    std::optional<std::string> updatedBy;
};

// Helper functions for timestamp conversion
void to_json(json& j, const timestamp& t);
void from_json(const json& j, timestamp& t);
void to_json(json& j, const AuditInfo& audit);
void from_json(const json& j, AuditInfo& audit);

enum class Status {
    Active,
    Inactive,
    Archived
};

std::string statusToString(Status status);
Status stringToStatus(const std::string& str);

} // namespace warehouse::models

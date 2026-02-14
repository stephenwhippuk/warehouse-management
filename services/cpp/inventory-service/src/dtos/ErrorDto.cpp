#include "inventory/dtos/ErrorDto.hpp"
#include <stdexcept>
#include <regex>

namespace inventory {
namespace dtos {

ErrorDto::ErrorDto(const std::string& error,
                   const std::string& message,
                   const std::string& requestId,
                   const std::string& timestamp,
                   const std::string& path,
                   const std::optional<std::vector<json>>& details)
    : error_(error)
    , message_(message)
    , requestId_(requestId)
    , timestamp_(timestamp)
    , path_(path)
    , details_(details) {
    
    // Validate all required fields
    validateError(error_);
    validateMessage(message_);
    validateRequestId(requestId_);
    validateTimestamp(timestamp_);
    validatePath(path_);
}

void ErrorDto::validateError(const std::string& error) const {
    if (error.empty()) {
        throw std::invalid_argument("Error type cannot be empty");
    }
}

void ErrorDto::validateMessage(const std::string& message) const {
    if (message.empty()) {
        throw std::invalid_argument("Error message cannot be empty");
    }
}

void ErrorDto::validateRequestId(const std::string& requestId) const {
    // Validate UUID format
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(requestId, uuidRegex)) {
        throw std::invalid_argument("RequestId must be a valid UUID");
    }
}

void ErrorDto::validateTimestamp(const std::string& timestamp) const {
    // Validate ISO 8601 DateTime format (basic check)
    if (timestamp.empty()) {
        throw std::invalid_argument("Timestamp cannot be empty");
    }
    // Full ISO 8601 validation would be more complex; this is a basic check
    static const std::regex isoRegex(
        R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$)"
    );
    if (!std::regex_match(timestamp, isoRegex)) {
        throw std::invalid_argument("Timestamp must be in ISO 8601 format");
    }
}

void ErrorDto::validatePath(const std::string& path) const {
    if (path.empty()) {
        throw std::invalid_argument("Path cannot be empty");
    }
}

json ErrorDto::toJson() const {
    json j = {
        {"error", error_},
        {"message", message_},
        {"requestId", requestId_},
        {"timestamp", timestamp_},
        {"path", path_}
    };
    
    if (details_) {
        j["details"] = *details_;
    }
    
    return j;
}

} // namespace dtos
} // namespace inventory

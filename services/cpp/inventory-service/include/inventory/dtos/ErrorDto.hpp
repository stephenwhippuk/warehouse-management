#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace inventory {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Standard error response DTO
 * 
 * Conforms to ErrorDto contract v1.0
 */
class ErrorDto {
public:
    /**
     * @brief Construct error DTO with all required fields
     * @param error Error type
     * @param message Human-readable error message
     * @param requestId Request identifier for tracing
     * @param timestamp When the error occurred (ISO 8601 format)
     * @param path Request URI path
     * @param details Optional detailed error information
     */
    ErrorDto(const std::string& error,
             const std::string& message,
             const std::string& requestId,
             const std::string& timestamp,
             const std::string& path,
             const std::optional<std::vector<json>>& details = std::nullopt);

    // Getters (immutable)
    std::string getError() const { return error_; }
    std::string getMessage() const { return message_; }
    std::string getRequestId() const { return requestId_; }
    std::string getTimestamp() const { return timestamp_; }
    std::string getPath() const { return path_; }
    std::optional<std::vector<json>> getDetails() const { return details_; }

    // Serialization
    json toJson() const;

private:
    std::string error_;
    std::string message_;
    std::string requestId_;
    std::string timestamp_;
    std::string path_;
    std::optional<std::vector<json>> details_;

    // Validation
    void validateError(const std::string& error) const;
    void validateMessage(const std::string& message) const;
    void validateRequestId(const std::string& requestId) const;
    void validateTimestamp(const std::string& timestamp) const;
    void validatePath(const std::string& path) const;
};

} // namespace dtos
} // namespace inventory

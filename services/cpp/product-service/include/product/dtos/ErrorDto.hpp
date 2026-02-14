#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace product::dtos {

/**
 * @brief Standard error response DTO
 */
class ErrorDto {
public:
    ErrorDto(const std::string& error,
             const std::string& message,
             const std::optional<std::string>& details = std::nullopt);

    std::string getError() const { return error_; }
    std::string getMessage() const { return message_; }
    std::optional<std::string> getDetails() const { return details_; }
    
    json toJson() const;

private:
    std::string error_;
    std::string message_;
    std::optional<std::string> details_;
};

}  // namespace product::dtos

#include "product/dtos/ErrorDto.hpp"
#include <stdexcept>

namespace product::dtos {

ErrorDto::ErrorDto(const std::string& error,
                   const std::string& message,
                   const std::optional<std::string>& details)
    : error_(error)
    , message_(message)
    , details_(details) {
    
    if (error_.empty()) {
        throw std::invalid_argument("error cannot be empty");
    }
    if (message_.empty()) {
        throw std::invalid_argument("message cannot be empty");
    }
}

json ErrorDto::toJson() const {
    json j = {
        {"error", error_},
        {"message", message_}
    };
    
    if (details_) {
        j["details"] = *details_;
    }
    
    return j;
}

}  // namespace product::dtos

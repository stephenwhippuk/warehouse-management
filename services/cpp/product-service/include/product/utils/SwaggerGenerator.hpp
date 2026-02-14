#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace product::utils {

/**
 * @brief OpenAPI/Swagger specification generator
 */
class SwaggerGenerator {
public:
    static json generateSpec(const std::string& version = "1.0.0");
};

}  // namespace product::utils

#pragma once

#include "http-framework/ControllerBase.hpp"
#include "http-framework/HttpContext.hpp"

namespace warehouse::controllers {

/**
 * @brief Swagger/OpenAPI spec endpoint handler
 */
class SwaggerController : public http::ControllerBase {
public:
    SwaggerController();

private:
    std::string handleSwagger(http::HttpContext& ctx);
};

}  // namespace warehouse::controllers

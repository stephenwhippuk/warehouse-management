#pragma once

#include "http-framework/ControllerBase.hpp"
#include "http-framework/HttpContext.hpp"
#include <string>

namespace product::controllers {

/**
 * @brief Health check endpoint handler
 */
class HealthController : public http::ControllerBase {
public:
    HealthController();

private:
    std::string handleHealth(http::HttpContext& ctx);
};

}  // namespace product::controllers

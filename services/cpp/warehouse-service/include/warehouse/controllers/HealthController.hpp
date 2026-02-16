#pragma once

#include "http-framework/ControllerBase.hpp"
#include "http-framework/HttpContext.hpp"

namespace warehouse::controllers {

/**
 * @brief Simple health check controller
 */
class HealthController : public http::ControllerBase {
public:
    HealthController();

private:
    std::string handleHealth(http::HttpContext& ctx);
};

}  // namespace warehouse::controllers

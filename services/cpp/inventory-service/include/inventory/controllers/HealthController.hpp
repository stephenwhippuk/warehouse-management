#pragma once

#include <http-framework/ControllerBase.hpp>

namespace inventory {
namespace controllers {

/**
 * @brief Simple health check controller
 *
 * Exposes a lightweight /health endpoint that reports basic
 * service status along with authentication metrics.
 */
class HealthController : public http::ControllerBase {
public:
    HealthController();

private:
    std::string handleHealthCheck(http::HttpContext& ctx);
};

} // namespace controllers
} // namespace inventory

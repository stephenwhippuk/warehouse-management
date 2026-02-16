#pragma once

#include <http-framework/ControllerBase.hpp>
#include <http-framework/HttpContext.hpp>
#include <http-framework/IServiceProvider.hpp>

namespace order::controllers {

/**
 * @brief Health check controller
 */
class HealthController : public http::ControllerBase {
public:
    explicit HealthController(http::IServiceProvider& provider);

private:
    std::string health(http::HttpContext& ctx);
};

} // namespace order::controllers

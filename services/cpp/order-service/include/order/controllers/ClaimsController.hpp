#pragma once

#include <http-framework/ControllerBase.hpp>
#include <http-framework/HttpContext.hpp>
#include <http-framework/IServiceProvider.hpp>

namespace order::controllers {

/**
 * @brief Claims controller for contract system
 */
class ClaimsController : public http::ControllerBase {
public:
    explicit ClaimsController(http::IServiceProvider& provider);

private:
    std::string getClaims(http::HttpContext& ctx);
};

} // namespace order::controllers

#pragma once

#include "http-framework/ControllerBase.hpp"
#include "http-framework/HttpContext.hpp"

namespace warehouse::controllers {

/**
 * @brief Controller for exposing service claims and contract support
 */
class ClaimsController : public http::ControllerBase {
public:
    ClaimsController();

private:
    std::string handleClaims(http::HttpContext& ctx);
};

}  // namespace warehouse::controllers

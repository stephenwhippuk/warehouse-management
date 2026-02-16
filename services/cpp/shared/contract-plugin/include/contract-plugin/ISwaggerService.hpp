#ifndef CONTRACT_PLUGIN_ISWAGGER_SERVICE_HPP
#define CONTRACT_PLUGIN_ISWAGGER_SERVICE_HPP

#include <nlohmann/json.hpp>
#include <optional>

namespace contract {

/**
 * @brief Interface for OpenAPI/Swagger specification generation
 * 
 * Generates OpenAPI 3.0 specifications from service contracts.
 */
class ISwaggerService {
public:
    virtual ~ISwaggerService() = default;

    /**
     * @brief Generate complete OpenAPI specification
     * @return OpenAPI spec JSON
     * @throws std::runtime_error if generation failed
     */
    virtual nlohmann::json generateSpec() = 0;
};

} // namespace contract

#endif // CONTRACT_PLUGIN_ISWAGGER_SERVICE_HPP

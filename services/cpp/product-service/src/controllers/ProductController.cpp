#include "product/controllers/ProductController.hpp"
#include "product/services/IProductService.hpp"
#include "product/utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace product::controllers {

ProductController::ProductController() 
    : http::ControllerBase("/api/v1/products") {
    
    // GET /api/v1/products (list all)
    Get("/", [this](http::HttpContext& ctx) {
        return this->handleGetAll(ctx);
    });
    
    // GET /api/v1/products/{id} (get by ID)
    Get("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleGetById(ctx);
    });
    
    // POST /api/v1/products (create)
    Post("/", [this](http::HttpContext& ctx) {
        return this->handleCreate(ctx);
    });
    
    // PUT /api/v1/products/{id} (update)
    Put("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleUpdate(ctx);
    });
    
    // DELETE /api/v1/products/{id} (delete)
    Delete("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleDelete(ctx);
    });
}

std::string ProductController::handleGetAll(http::HttpContext& ctx) {
    // Get query parameters
    int page = ctx.queryParams.getInt("page").value_or(1);
    int pageSize = ctx.queryParams.getInt("pageSize").value_or(50);
    
    // Resolve service from request scope
    auto service = ctx.getService<services::IProductService>();
    
    // Get products - exceptions caught by ErrorHandlingMiddleware
    auto list = service->getAll(page, pageSize);
    
    return list.toJson().dump();
}

std::string ProductController::handleGetById(http::HttpContext& ctx) {
    // Get route parameter
    std::string id = ctx.routeParams["id"];
    
    // Resolve service from request scope
    auto service = ctx.getService<services::IProductService>();
    
    // Get product
    auto product = service->getById(id);
    
    if (!product) {
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        throw std::runtime_error("Product not found: " + id);
    }
    
    return product->toJson().dump();
}

std::string ProductController::handleCreate(http::HttpContext& ctx) {
    // Parse JSON body
    json body = ctx.getBodyAsJson();
    
    std::string sku = body.at("sku").get<std::string>();
    std::string name = body.at("name").get<std::string>();
    std::optional<std::string> description = std::nullopt;
    std::optional<std::string> category = std::nullopt;
    
    if (body.contains("description") && !body["description"].is_null()) {
        description = body["description"].get<std::string>();
    }
    if (body.contains("category") && !body["category"].is_null()) {
        category = body["category"].get<std::string>();
    }
    
    // Resolve service from request scope
    auto service = ctx.getService<services::IProductService>();
    
    // Create product - exceptions caught by ErrorHandlingMiddleware
    auto product = service->create(sku, name, description, category);
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
    return product.toJson().dump();
}

std::string ProductController::handleUpdate(http::HttpContext& ctx) {
    // Get route parameter
    std::string id = ctx.routeParams["id"];
    
    // Parse JSON body
    json body = ctx.getBodyAsJson();
    
    std::string name = body.at("name").get<std::string>();
    std::string status = body.at("status").get<std::string>();
    std::optional<std::string> description = std::nullopt;
    std::optional<std::string> category = std::nullopt;
    
    if (body.contains("description") && !body["description"].is_null()) {
        description = body["description"].get<std::string>();
    }
    if (body.contains("category") && !body["category"].is_null()) {
        category = body["category"].get<std::string>();
    }
    
    // Resolve service from request scope
    auto service = ctx.getService<services::IProductService>();
    
    // Update product - exceptions caught by ErrorHandlingMiddleware
    auto product = service->update(id, name, description, category, status);
    
    return product.toJson().dump();
}

std::string ProductController::handleDelete(http::HttpContext& ctx) {
    // Get route parameter
    std::string id = ctx.routeParams["id"];
    
    // Resolve service from request scope
    auto service = ctx.getService<services::IProductService>();
    
    // Delete product - exceptions caught by ErrorHandlingMiddleware
    bool deleted = service->deleteById(id);
    
    if (!deleted) {
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        throw std::runtime_error("Product not found: " + id);
    }
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
    return "";  // No content for 204
}

}  // namespace product::controllers

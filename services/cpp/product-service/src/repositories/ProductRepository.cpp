#include "product/repositories/ProductRepository.hpp"
#include <stdexcept>

using namespace std::string_literals;

namespace product::repositories {

ProductRepository::ProductRepository(http::IServiceProvider& provider)
    : db_(provider.getService<utils::Database>()) {
}

std::optional<models::Product> ProductRepository::findById(const std::string& id) {
    try {
        pqxx::work txn(*db_->getConnection());
        auto result = txn.exec_params(
            "SELECT id, sku, name, description, category, status FROM products WHERE id = $1",
            id
        );
        txn.commit();
        
        if (result.empty()) {
            return std::nullopt;
        }
        
        return rowToProduct(result[0]);
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error finding product: "s + e.what());
    }
}

std::optional<models::Product> ProductRepository::findBySku(const std::string& sku) {
    try {
        pqxx::work txn(*db_->getConnection());
        auto result = txn.exec_params(
            "SELECT id, sku, name, description, category, status FROM products WHERE sku = $1",
            sku
        );
        txn.commit();
        
        if (result.empty()) {
            return std::nullopt;
        }
        
        return rowToProduct(result[0]);
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error finding product: "s + e.what());
    }
}

std::vector<models::Product> ProductRepository::findAll() {
    try {
        pqxx::work txn(*db_->getConnection());
        auto result = txn.exec("SELECT id, sku, name, description, category, status FROM products ORDER BY sku");
        txn.commit();
        
        std::vector<models::Product> products;
        for (auto row : result) {
            products.push_back(rowToProduct(row));
        }
        return products;
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error finding products: "s + e.what());
    }
}

std::vector<models::Product> ProductRepository::findActive() {
    try {
        pqxx::work txn(*db_->getConnection());
        auto result = txn.exec("SELECT id, sku, name, description, category, status FROM products WHERE status = 'active' ORDER BY sku");
        txn.commit();
        
        std::vector<models::Product> products;
        for (auto row : result) {
            products.push_back(rowToProduct(row));
        }
        return products;
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error finding active products: "s + e.what());
    }
}

models::Product ProductRepository::create(const models::Product& product) {
    try {
        pqxx::work txn(*db_->getConnection());
        txn.exec_params(
            "INSERT INTO products (id, sku, name, description, category, status) VALUES ($1, $2, $3, $4, $5, $6)",
            product.getId(),
            product.getSku(),
            product.getName(),
            product.getDescription().value_or(nullptr),
            product.getCategory().value_or(nullptr),
            [&]() {
                switch (product.getStatus()) {
                    case models::Product::Status::ACTIVE: return "active";
                    case models::Product::Status::INACTIVE: return "inactive";
                    case models::Product::Status::DISCONTINUED: return "discontinued";
                }
            }()
        );
        txn.commit();
        return product;
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error creating product: "s + e.what());
    }
}

models::Product ProductRepository::update(const models::Product& product) {
    try {
        pqxx::work txn(*db_->getConnection());
        txn.exec_params(
            "UPDATE products SET name = $2, description = $3, category = $4, status = $5 WHERE id = $1",
            product.getId(),
            product.getName(),
            product.getDescription().value_or(nullptr),
            product.getCategory().value_or(nullptr),
            [&]() {
                switch (product.getStatus()) {
                    case models::Product::Status::ACTIVE: return "active";
                    case models::Product::Status::INACTIVE: return "inactive";
                    case models::Product::Status::DISCONTINUED: return "discontinued";
                }
            }()
        );
        txn.commit();
        return product;
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error updating product: "s + e.what());
    }
}

bool ProductRepository::deleteById(const std::string& id) {
    try {
        pqxx::work txn(*db_->getConnection());
        auto result = txn.exec_params("DELETE FROM products WHERE id = $1", id);
        txn.commit();
        return result.affected_rows() > 0;
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error deleting product: "s + e.what());
    }
}

models::Product ProductRepository::rowToProduct(const pqxx::row& row) {
    std::string statusStr = row["status"].as<std::string>();
    models::Product::Status status;
    
    if (statusStr == "active") {
        status = models::Product::Status::ACTIVE;
    } else if (statusStr == "inactive") {
        status = models::Product::Status::INACTIVE;
    } else if (statusStr == "discontinued") {
        status = models::Product::Status::DISCONTINUED;
    } else {
        throw std::runtime_error("Invalid product status: " + statusStr);
    }
    
    std::optional<std::string> description = std::nullopt;
    std::optional<std::string> category = std::nullopt;
    
    if (!row["description"].is_null()) {
        description = row["description"].as<std::string>();
    }
    if (!row["category"].is_null()) {
        category = row["category"].as<std::string>();
    }
    
    return models::Product(
        row["id"].as<std::string>(),
        row["sku"].as<std::string>(),
        row["name"].as<std::string>(),
        description,
        category,
        status
    );
}

}  // namespace product::repositories

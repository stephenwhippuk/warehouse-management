#include "product/utils/Database.hpp"
#include "product/utils/Logger.hpp"

namespace product::utils {

std::shared_ptr<pqxx::connection> Database::connection_;
std::string Database::connectionString_;

void Database::connect(const std::string& connectionString) {
    try {
        connectionString_ = connectionString;
        connection_ = std::make_shared<pqxx::connection>(connectionString);
        
        if (connection_->is_open()) {
            if (auto logger = Logger::getLogger()) logger->info("Connected to PostgreSQL database");
        } else {
            throw std::runtime_error("Failed to open database connection");
        }
    } catch (const std::exception& e) {
        if (auto logger = Logger::getLogger()) logger->error("Database connection failed: {}", e.what());
        throw;
    }
}

void Database::disconnect() {
    if (connection_ && connection_->is_open()) {
        // pqxx::connection closes automatically when shared_ptr is destroyed
        // Just ensure we're logged
        if (auto logger = Logger::getLogger()) logger->info("Disconnected from database");
    }
}

std::shared_ptr<pqxx::connection> Database::getConnection() {
    if (!connection_ || !connection_->is_open()) {
        throw std::runtime_error("Database connection not initialized or closed");
    }
    return connection_;
}

}  // namespace product::utils

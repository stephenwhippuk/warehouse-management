#include "inventory/utils/Database.hpp"
#include <stdexcept>

namespace inventory {
namespace utils {

std::shared_ptr<pqxx::connection> Database::connection_ = nullptr;

std::shared_ptr<pqxx::connection> Database::connect(const std::string& connectionString) {
    try {
        connection_ = std::make_shared<pqxx::connection>(connectionString);
        if (!connection_->is_open()) {
            throw std::runtime_error("Failed to open database connection");
        }
        return connection_;
    } catch (const std::exception& e) {
        throw std::runtime_error("Database connection error: " + std::string(e.what()));
    }
}

void Database::disconnect() {
    if (connection_ && connection_->is_open()) {
        connection_->close();
    }
    connection_.reset();
}

std::shared_ptr<pqxx::connection> Database::getConnection() {
    if (!connection_ || !connection_->is_open()) {
        throw std::runtime_error("No active database connection");
    }
    return connection_;
}

void Database::releaseConnection(std::shared_ptr<pqxx::connection> conn) {
    // TODO: Implement connection pooling
    // For now, we're using a single shared connection
}

} // namespace utils
} // namespace inventory

#include "inventory/utils/Database.hpp"
#include "inventory/utils/Logger.hpp"
#include <sstream>
#include <stdexcept>

namespace inventory {
namespace utils {

Database::Database(const Config& config)
    : config_(config)
    , connectionString_(buildConnectionString()) {
}

Database::~Database() {
    disconnect();
}

std::string Database::buildConnectionString() const {
    std::ostringstream oss;
    oss << "host=" << config_.host
        << " port=" << config_.port
        << " dbname=" << config_.database
        << " user=" << config_.user
        << " password=" << config_.password;
    return oss.str();
}

bool Database::connect() {
    try {
        connection_ = std::make_shared<pqxx::connection>(connectionString_);
        Logger::info("Database connected successfully");
        return true;
    } catch (const pqxx::broken_connection& e) {
        Logger::error("Database connection failed: {}", e.what());
        return false;
    }
}

void Database::disconnect() {
    if (connection_ && connection_->is_open()) {
        connection_->close();
        Logger::info("Database disconnected");
    }
}

bool Database::isConnected() const {
    return connection_ && connection_->is_open();
}

std::unique_ptr<pqxx::work> Database::beginTransaction() {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    return std::make_unique<pqxx::work>(*connection_);
}

pqxx::result Database::execute(const std::string& query) {
    auto txn = beginTransaction();
    auto result = txn->exec(query);
    txn->commit();
    return result;
}

pqxx::result Database::executeParams(const std::string& query, const std::vector<std::string>& params) {
    (void)params;
    // TODO: Implement parameterized query execution
    auto txn = beginTransaction();
    auto result = txn->exec(query); // Temporary: doesn't use params
    txn->commit();
    return result;
}

void Database::prepare(const std::string& name, const std::string& query) {
    if (!isConnected()) {
        throw std::runtime_error("Database not connected");
    }
    connection_->prepare(name, query);
}

pqxx::result Database::executePrepared(const std::string& name, const std::vector<std::string>& params) {
    (void)params;
    // TODO: Implement prepared statement execution with params
    // For now, just execute as a simple query (stub)
    auto txn = beginTransaction();
    auto result = txn->exec(name);
    txn->commit();
    return result;
}

std::shared_ptr<pqxx::connection> Database::getConnection() {
    // TODO: Implement connection pooling
    if (!isConnected()) {
        connect();
    }
    return connection_;
}

void Database::releaseConnection(std::shared_ptr<pqxx::connection> conn) {
    (void)conn;
    // TODO: Implement connection pool return logic
}

} // namespace utils
} // namespace inventory

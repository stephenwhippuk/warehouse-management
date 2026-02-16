#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>

namespace warehouse::utils {

/**
 * @brief Database connection and query utilities for PostgreSQL
 */
class Database {
public:
    struct Config {
        std::string host = "localhost";
        int port = 5432;
        std::string database = "warehouse_db";
        std::string user = "warehouse";
        std::string password;
        int maxConnections = 10;
    };
    
    explicit Database(const Config& config);
    ~Database();
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    // Transaction support
    std::unique_ptr<pqxx::work> beginTransaction();
    
    // Query execution
    pqxx::result execute(const std::string& query);
    pqxx::result executeParams(const std::string& query, const std::vector<std::string>& params);
    
    // Prepared statement support
    void prepare(const std::string& name, const std::string& query);
    pqxx::result executePrepared(const std::string& name, const std::vector<std::string>& params = {});
    
    // Connection pool (simple implementation)
    std::shared_ptr<pqxx::connection> getConnection();
    void releaseConnection(std::shared_ptr<pqxx::connection> conn);

private:
    Config config_;
    std::string connectionString_;
    std::shared_ptr<pqxx::connection> connection_;
    
    std::string buildConnectionString() const;
};

} // namespace warehouse::utils

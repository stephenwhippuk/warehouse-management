#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>

namespace product::utils {

/**
 * @brief Database connection management
 */
class Database {
public:
    static void connect(const std::string& connectionString);
    static void disconnect();
    static std::shared_ptr<pqxx::connection> getConnection();
    
private:
    static std::shared_ptr<pqxx::connection> connection_;
    static std::string connectionString_;
};

}  // namespace product::utils

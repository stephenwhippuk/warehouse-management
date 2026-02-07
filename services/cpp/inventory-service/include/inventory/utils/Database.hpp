#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>

namespace inventory {
namespace utils {

class Database {
public:
    static std::shared_ptr<pqxx::connection> connect(const std::string& connectionString);
    static void disconnect();
    
    // Connection pool methods (TODO: Implement connection pooling)
    static std::shared_ptr<pqxx::connection> getConnection();
    static void releaseConnection(std::shared_ptr<pqxx::connection> conn);
    
private:
    static std::shared_ptr<pqxx::connection> connection_;
};

} // namespace utils
} // namespace inventory

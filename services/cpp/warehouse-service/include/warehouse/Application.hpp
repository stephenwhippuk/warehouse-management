#pragma once

#include "Server.hpp"
#include "utils/Config.hpp"
#include "utils/Database.hpp"
#include "utils/Logger.hpp"
#include <memory>
#include <string>

namespace warehouse {

/**
 * @brief Main application class
 */
class Application {
public:
    Application();
    ~Application();
    
    bool initialize(const std::string& configFile = "config/application.json");
    void run();
    void shutdown();
    
    static Application& instance();

private:
    bool loadConfiguration(const std::string& configFile);
    bool initializeDatabase();
    bool initializeServices();
    bool initializeServer();
    void setupSignalHandlers();
    
    std::unique_ptr<Server> server_;
    std::shared_ptr<utils::Database> database_;
    bool initialized_ = false;
    bool running_ = false;
    
    static Application* instance_;
};

} // namespace warehouse

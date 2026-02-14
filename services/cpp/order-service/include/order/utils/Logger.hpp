#pragma once

#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace order::utils {

/**
 * @brief Logging utility wrapper around spdlog
 */
class Logger {
public:
    enum class Level {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };
    
    static void init(const std::string& logFile = "",
                    Level level = Level::Info,
                    bool consoleOutput = true);
    
    static std::shared_ptr<spdlog::logger> get(const std::string& name = "order");
    
    static void setLevel(Level level);
    
    // Convenience methods
    template<typename... Args>
    static void trace(Args&&... args) {
        get()->trace(std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void debug(Args&&... args) {
        get()->debug(std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void info(Args&&... args) {
        get()->info(std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warn(Args&&... args) {
        get()->warn(std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(Args&&... args) {
        get()->error(std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void critical(Args&&... args) {
        get()->critical(std::forward<Args>(args)...);
    }
    
private:
    static std::shared_ptr<spdlog::logger> logger_;
    static spdlog::level::level_enum convertLevel(Level level);
};

} // namespace order::utils

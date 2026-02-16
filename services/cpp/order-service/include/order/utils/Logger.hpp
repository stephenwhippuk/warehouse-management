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
    
    // Convenience methods with runtime format strings
    template<typename... Args>
    static void trace(const std::string& fmt, Args&&... args) {
        get()->trace(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void debug(const std::string& fmt, Args&&... args) {
        get()->debug(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void info(const std::string& fmt, Args&&... args) {
        get()->info(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warn(const std::string& fmt, Args&&... args) {
        get()->warn(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(const std::string& fmt, Args&&... args) {
        get()->error(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void critical(const std::string& fmt, Args&&... args) {
        get()->critical(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }
    
private:
    static std::shared_ptr<spdlog::logger> logger_;
    static spdlog::level::level_enum convertLevel(Level level);
};

} // namespace order::utils

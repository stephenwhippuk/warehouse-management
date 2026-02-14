#include "product/utils/Logger.hpp"

namespace product::utils {

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::initialize(const std::string& level) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::vector<spdlog::sink_ptr> sinks{console_sink};
    logger_ = std::make_shared<spdlog::logger>("product-service", sinks.begin(), sinks.end());
    logger_->flush_on(spdlog::level::err);
    
    if (level == "debug") {
        logger_->set_level(spdlog::level::debug);
    } else if (level == "warn") {
        logger_->set_level(spdlog::level::warn);
    } else if (level == "err") {
        logger_->set_level(spdlog::level::err);
    } else {
        logger_->set_level(spdlog::level::info);
    }
    
    spdlog::register_logger(logger_);
}

}  // namespace product::utils

#include "utils.hpp"
#include <chrono>
#include <filesystem>

std::shared_ptr<spdlog::logger> create_logger(const std::string &logger_name) {
    // 创建文件夹
    std::filesystem::path log_dir("./logs");
    if (!std::filesystem::exists(log_dir)) {
        if (!std::filesystem::create_directories(log_dir)) {
            throw std::runtime_error("Failed to create log directory");
        }
    }
    // 构建日志文件路径
    std::filesystem::path log_file = log_dir / (logger_name + ".log");
    // 创建单独的同步 logger，truncate 模式每次覆盖文件
    auto logger = spdlog::basic_logger_st(logger_name, log_file.string(), true);
    // 设置 logger 的格式和级别
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
    logger->set_level(spdlog::level::info);
    spdlog::flush_every(std::chrono::milliseconds(100));
    return logger;
}
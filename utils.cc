#include "utils.hpp"
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <stdexcept>

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

int read_and_compare(const std::string &file_path, std::streamoff offset, std::streamsize length, const void *compare_data) {
    // 检查文件是否存在
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error(fmt::format("File does not exist: {}",file_path));
    }
    // 获取文件大小
    std::streamsize file_size = std::filesystem::file_size(file_path);

    // 如果偏移位置超过文件大小，返回0
    if (offset + length >= file_size) {
        return 0;
    }

    // 打开文件
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error(fmt::format("Failed to open file: {}",file_path));
    }

    // 移动到指定偏移位置
    file.seekg(offset);

    // 读取指定长度的数据
    std::vector<char> buffer(length);
    file.read(buffer.data(), length);
    // 比较内存内容
    if (std::memcmp(buffer.data(), compare_data, length) == 0) {
        return 1;// 数据一致
    } else {
        return 0;// 数据不一致，返回文件长度
    }
}
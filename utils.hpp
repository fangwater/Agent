#ifndef UTILS_HPP
#define UTILS_HPP
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
using Logger = std::shared_ptr<spdlog::logger>;
std::shared_ptr<spdlog::logger> create_logger(const std::string &logger_name);


#endif
#ifndef UTILS_HPP
#define UTILS_HPP
#include <cstring>// For std::memcmp
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
using Logger = std::shared_ptr<spdlog::logger>;
std::shared_ptr<spdlog::logger> create_logger(const std::string &logger_name);

// 读取文件指定偏移位置和长度，若文件大小不足返回文件长度，若相同则返回0
/**
 * @brief 用于agent的校验，只校验index，因为先写文件后写索引，索引存在即认为数据完整
 * 
 * @param file_path 
 * @param offset 
 * @param length 
 * @param compare_data 
 * @return std::streamsize 
 */
int read_and_compare(const std::string &file_path, std::streamoff offset, std::streamsize length, const void *compare_data);


#endif
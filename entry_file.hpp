#ifndef ENTRY_FILE_HPP
#define ENTRY_FILE_HPP
#include <fcntl.h>
#include <filesystem>
#include <fmt/format.h>
#include <string>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

class EntryFile {
public:
    EntryFile(const std::string &path, size_t truncated_size, size_t entry_length);
    EntryFile() = delete;

    size_t write(const char *buffer, int count = 1);
    size_t writev(const std::vector<char *> &buffers, const std::vector<int>& counts);
    void backup_file();

private:
    std::filesystem::path file_path_;
    int64_t entry_count_;
    int64_t entry_length_;

    void truncate_file(size_t truncated_size);
    void create_or_clean_file();
};
#endif// ENTRY_FILE_HPP
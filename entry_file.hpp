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
    EntryFile(const std::string &dir, const std::string &filename, size_t truncated_size, size_t entry_length);
    EntryFile() = delete;

    size_t write(const char *buffer);
    size_t writev(const std::vector<const char *> &buffers);
    void backup_file();

private:
    std::filesystem::path file_path_;
    int64_t entry_count_;
    int64_t entry_length_;

    void truncate_file(size_t truncated_size);
    void create_or_clean_file();
};
#endif// ENTRY_FILE_HPP
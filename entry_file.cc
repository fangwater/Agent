
#include "entry_file.hpp"
#include <fstream>
#include <stdexcept>
size_t EntryFile::write(const char *buffer) {
    int fd = open(file_path_.c_str(), O_APPEND | O_WRONLY);
    if (fd == -1) {
        throw std::runtime_error("Failed to open file for writing.");
    }

    ssize_t written = ::write(fd, buffer, entry_length_);
    if (written == -1) {
        close(fd);
        throw std::runtime_error("Failed to write to file.");
    }

    close(fd);
    entry_count_++;
    return written;
}

size_t EntryFile::writev(const std::vector<const char *> &buffers) {
    std::vector<struct iovec> iov(buffers.size());

    for (size_t i = 0; i < buffers.size(); ++i) {
        iov[i].iov_base = (void *) buffers[i];
        iov[i].iov_len = entry_length_;
    }

    int fd = open(file_path_.c_str(), O_APPEND | O_WRONLY);
    if (fd == -1) {
        throw std::runtime_error("Failed to open file for writing.");
    }

    ssize_t written = ::writev(fd, iov.data(), iov.size());
    if (written == -1) {
        close(fd);
        throw std::runtime_error("Failed to writev to file.");
    }

    close(fd);
    entry_count_ += buffers.size();
    return written;
}

void EntryFile::backup_file() {
    auto backup_path = file_path_;
    backup_path += ".bk";
    std::filesystem::copy_file(file_path_, backup_path, std::filesystem::copy_options::overwrite_existing);
}

EntryFile::EntryFile(const std::string &dir, const std::string &filename, size_t truncated_size, size_t entry_length)
    : entry_length_(entry_length), entry_count_(0) {

    file_path_ = std::filesystem::path(dir) / filename;

    std::filesystem::create_directories(dir);

    if (truncated_size > 0) {
        truncate_file(truncated_size);
    } else {
        create_or_clean_file();
    }
}

void EntryFile::truncate_file(size_t truncated_size) {
    if (!std::filesystem::exists(file_path_)) {
        throw std::runtime_error(fmt::format("File does not exist: {}", file_path_.string()));
    }

    auto file_size = std::filesystem::file_size(file_path_);
    if (file_size == truncated_size) {
        entry_count_ = truncated_size / entry_length_;
        return;
    } else if (file_size > truncated_size) {
        backup_file();
        std::ofstream file(file_path_, std::ios::in | std::ios::out | std::ios::binary);
        file.seekp(truncated_size);
        file.close();
        entry_count_ = truncated_size / entry_length_;
    } else {
        throw std::runtime_error("File size is smaller than the truncated size.");
    }
}

void EntryFile::create_or_clean_file() {
    if (std::filesystem::exists(file_path_)) {
        std::filesystem::remove(file_path_);
    }
    std::ofstream file(file_path_, std::ios::out | std::ios::binary);
    file.close();
}

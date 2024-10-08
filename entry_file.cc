
#include "entry_file.hpp"
#include <fstream>
#include <numeric>
#include <stdexcept>
#include <vector>
size_t EntryFile::write(const char *buffer, int count) {
    int fd = open(file_path_.c_str(), O_APPEND | O_WRONLY);
    if (fd == -1) {
        throw std::runtime_error("Failed to open file for writing.");
    }

    ssize_t written = ::write(fd, buffer, entry_length_*count);
    if (written == -1) {
        close(fd);
        throw std::runtime_error("Failed to write to file.");
    }
    if (::fsync(fd) == -1) {
        close(fd);
        throw std::runtime_error("Failed to flush data to disk.");
    }
    close(fd);
    entry_count_ += count;
    return written;
}

size_t EntryFile::writev(const std::vector<char *> &buffers, const std::vector<int>& counts) {
    std::vector<struct iovec> iov(buffers.size());

    for (size_t i = 0; i < buffers.size(); ++i) {
        iov[i].iov_base = (void *) buffers[i];
        iov[i].iov_len = entry_length_ * counts[i];
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
    if (::fsync(fd) == -1) {
        close(fd);
        throw std::runtime_error("Failed to flush data to disk.");
    }
    close(fd);
    entry_count_ += std::accumulate(counts.begin(),counts.end(),0);
    return written;
}

void EntryFile::backup_file() {
    auto backup_path = file_path_;
    backup_path += ".bk";
    std::filesystem::copy_file(file_path_, backup_path, std::filesystem::copy_options::overwrite_existing);
}

EntryFile::EntryFile(const std::string &path, size_t truncated_size, size_t entry_length)
    : entry_length_(entry_length), entry_count_(0) {

    file_path_ = std::filesystem::path(path);
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

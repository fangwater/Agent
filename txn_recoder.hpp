#ifndef TXN_RECORDER_HPP
#define TXN_RECORDER_HPP

#include "entry_file.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "txn_entry.hpp"

class TxnRecorder {
public:
    TxnRecorder(std::string dir);
    off_t query_offset_by_data_seq_num(int64_t data_seq_number) const;
    off_t query_offset_by_txn_id(int64_t txn_id) const;
    int64_t get_processed_txn_id() const;

private:
    void init_index_file(const std::string &idx_path);
    void verify_log_file(const std::string &log_path);

    std::vector<TxnEntry> entries_;
    std::vector<int64_t> data_count_prefix_sum_;
    int64_t processed_txn_id_;
    std::shared_ptr<EntryFile> idx_;
    std::shared_ptr<EntryFile> content_;
    const char *index_name = "txn_index.bin";
    const char *log_name = "txn_content.bin";
};

/**
 * @brief Construct a new Txn Recorder:: Txn Recorder object
 * 
 * @param idx_path 索引文件路径
 * @param log_path 日志文件路径
 */
TxnRecorder::TxnRecorder(std::string dir) {
    //判断文件夹是否存在，不存在则创建dir
    if (std::filesystem::exists(dir)) {
        //初始化索引文件
        //const std::string &dir, const std::string &filename, size_t truncated_size, size_t entry_length
    } else {
        //不存在文件夹，代表首次启动，截断长度为0
        idx_ = std::shared_ptr<EntryFile>(new EntryFile(dir, index_name,0,sizeof(TxnEntry)));
    }

    
}

#endif// TXN_RECORDER_HPP

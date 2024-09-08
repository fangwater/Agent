#ifndef TXN_RECORDER_HPP
#define TXN_RECORDER_HPP
#include "dsp_message.hpp"
#include "entry_file.hpp"
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <string>
#include <vector>
#include "txn_entry.hpp"
#include "utils.hpp"
#include <spdlog/spdlog.h>

class TxnRecorder {
public:
    TxnRecorder(std::string dir, Logger logger);
    void append_txn_entry(const TxnEntry& entry);
    int64_t get_processed_txn_id() const;
    int64_t get_processed_total_data_count() const;
    void write_dsp_message(std::shared_ptr<DspMessage> msg);

private:
    void init_index_file();
    void verify_log_file();

    std::vector<TxnEntry> entries_;
    std::vector<int64_t> data_count_prefix_sum_;
    int64_t processed_txn_id_;
    std::shared_ptr<EntryFile> idx_;
    std::shared_ptr<EntryFile> content_;
    const char *index_name = "txn_index";
    const char *log_name = "txn_content";
    std::string idx_path_;
    std::string content_path_;
    Logger logger_;
};


#endif// TXN_RECORDER_HPP

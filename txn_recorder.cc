#include "txn_recorder.hpp"
#include "dsp_message.hpp"
#include "utils.hpp"
#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
void TxnRecorder::write_dsp_message(std::shared_ptr<DspMessage> msg) {
    msg->write(idx_, content_);
}


void TxnRecorder::append_txn_entry(const TxnEntry &entry) {
    entries_.push_back(entry);
    int64_t pre_count = data_count_prefix_sum_.back();
    data_count_prefix_sum_.push_back(pre_count + entry.dataCnt);
    processed_txn_id_++;
}

int64_t TxnRecorder::get_processed_txn_id() const {
    return processed_txn_id_;
}

int64_t TxnRecorder::get_processed_total_data_count() const {
    return data_count_prefix_sum_.empty() ? 0 : data_count_prefix_sum_.back();
}
/**
 * @brief Construct a new Txn Recorder:: Txn Recorder object
 * 
 * @param idx_path 索引文件路径
 * @param log_path 日志文件路径
 */
TxnRecorder::TxnRecorder(std::string dir,Logger logger):processed_txn_id_(0),logger_(logger) {
    idx_path_ = fmt::format("{}/{}", dir, index_name);
    content_path_ = fmt::format("{}/{}", dir, log_name);
    //判断文件夹是否存在，不存在则创建dir
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
        logger_->info("create txn log dir:{}",dir.c_str());
    }
    //如果索引文件存在，但是大小为0，则删除无意义的索引文件，视为不存在
    if (std::filesystem::exists(idx_path_) && std::filesystem::is_regular_file(idx_path_)) {
        if (!std::filesystem::file_size(idx_path_)) {
            logger_->info("index file is empty, remove it.");
            std::filesystem::remove(idx_path_);
            std::filesystem::remove(content_path_);
        }
    }
    /**
     * @brief 启动时，查看文件状态
     * 1、若索引文件为0，删除，此时被视为不存在的状态
     * 2、因此索引文件只分为存在(至少包含一条数据)和不存在两种情况
     */
    if (!std::filesystem::exists(idx_path_)) {
        //代表首次启动，截断长度为0
        idx_ = std::shared_ptr<EntryFile>(new EntryFile(idx_path_, 0, sizeof(TxnEntry)));
        content_ = std::shared_ptr<EntryFile>(new EntryFile(content_path_, 0, DspMessage::buffer_size));
    } else {
        //数据存在且不为0，需要读取数据
        init_index_file();
        verify_log_file();
    }
}


void TxnRecorder::verify_log_file() {
    //根据索引文件的读取结果，对log文件进行裁剪和校验
    //根据data_count 已直到buffer size 判断
    auto file_size = std::filesystem::file_size(log_name);
    auto expected_size = data_count_prefix_sum_.back() * DspMessage::buffer_size;
    if (file_size < expected_size) {
        throw std::runtime_error("log file size is less than expected, Possible file corruption");
    } else if (file_size > expected_size) {
        spdlog::info("log file size is largeer than expected, log file need tuncated");
    } else {
        spdlog::info("The log file size exactly matches index file expected .");
    }
    content_ = std::make_shared<EntryFile>(log_name, expected_size, DspMessage::buffer_size);
}

void TxnRecorder::init_index_file() {
    //索引文件存在，则读取索引内容
    auto file_size = std::filesystem::file_size(idx_path_);
    size_t txn_count = file_size / sizeof(TxnEntry);
    size_t valid_size = txn_count * sizeof(TxnEntry);
    if (valid_size != file_size) {
        logger_->info("Warning: File size ({}) is not a multiple of TxnEntry size ({}). Possible file corruption. Using only the {} txns.", file_size, sizeof(TxnEntry), txn_count);
    }
    entries_.resize(txn_count);
    data_count_prefix_sum_.resize(txn_count);
    // 读取文件到vector中
    std::ifstream file(idx_path_, std::ios::binary);
    if (!file.read(reinterpret_cast<char *>(entries_.data()), txn_count * sizeof(TxnEntry))) {
        throw std::runtime_error("Failed to read the txn entrys.");
    }
    //entry至少有一条数据
    processed_txn_id_ = entries_.back().txnId;
    logger_->info("last processed txnid is {}", processed_txn_id_);
    //前缀和，便于查询
    data_count_prefix_sum_[0] = entries_[0].dataCnt;
    for (int i = 1; i < entries_.size(); i++) {
        //前缀和计算的同时校验data_sq是否正确
        data_count_prefix_sum_[i] = data_count_prefix_sum_[i - 1] + entries_[i].dataCnt;
        if (data_count_prefix_sum_[i] != entries_[i].lstDataSqno) {
            throw std::runtime_error("error!");
        }
    }
    idx_ = std::shared_ptr<EntryFile>(new EntryFile(idx_path_, valid_size, sizeof(TxnEntry)));
}

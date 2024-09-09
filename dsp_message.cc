#include "dsp_message.hpp"
#include "spdlog/spdlog.h"
#include "txn_entry.hpp"
#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <stdexcept>
#include <vector>

// 辅助函数，用于创建 IntrnlMsgData
void add_repeat_data(message::intrnl::IntrnlMsg &msg, int dataCnt, const TxnEntry &entry) {
    message::intrnl::IntrnlMsgData *data = msg.add_intrnmsgdata();
    int64_t msg_len = sizeof(TxnEntry) + dataCnt * DspMessage::buffer_size;
    std::vector<char> buffer(msg_len);
    char *buffer_ptr = buffer.data();
    std::memcpy(buffer_ptr, &entry, sizeof(entry));
    data->set_data(buffer.data(), buffer.size());
}

// 创建 IntrnlMsg
message::intrnl::IntrnlMsg create_IntrnlMsg_with_repeat(int totalDataCnt, int64_t &txnid, int64_t& dataSqno) {
    txnid++;
    dataSqno++;
    message::intrnl::IntrnlMsg msg;
    constexpr int cnt_max = 3;
    int remainingDataCnt = totalDataCnt;
    int currentSqno = dataSqno;
    TxnEntry entry;
    entry.txnId = txnid;
    entry.frstDataSqno = dataSqno;
    entry.lstDataSqno = dataSqno + totalDataCnt - 1;
    while (remainingDataCnt > 0) {
        int currentDataCnt = std::min(cnt_max, remainingDataCnt);
        add_repeat_data(msg, currentDataCnt, entry);
        // 更新 remainingDataCnt 和 currentSqno
        remainingDataCnt -= currentDataCnt;
        currentSqno += currentDataCnt;
    }
    dataSqno += totalDataCnt - 1;
    spdlog::info("{}",entry.to_string());
    return msg;
}

void DspMessage::write(std::shared_ptr<EntryFile> index_file, std::shared_ptr<EntryFile> log_file) {
    if (!index_file) {
        throw std::runtime_error("ptr to idx file is null");
    }
    if (!log_file) {
        throw std::runtime_error("ptr to log file is null");
    }
    /**
     * @brief index只写一条，每个msg对应一个io，但是通过writev一次性写入
     * 先写log content部分，查看是否写入成功, 若写失败按照索引进行恢复
     */
    index_file->write(reinterpret_cast<char *>(&txn_entry_));
    log_file->writev(buffers_, data_counts_);
}

void DspMessage::process(int64_t processed_txn_id) {
    txn_entry_ = *reinterpret_cast<const TxnEntry *>(pbIntrnlReq_.intrnmsgdata().begin()->data().c_str());
    txn_entry_.msgCnt = 0;
    txn_entry_.dataCnt = 0;
    for (auto iter = pbIntrnlReq_.intrnmsgdata().begin(); iter != pbIntrnlReq_.intrnmsgdata().end(); iter++) {
        char *msg_data_ptr = const_cast<char *>(iter->data().c_str());
        TxnEntry *txn_entry_ptr = reinterpret_cast<TxnEntry *>(msg_data_ptr);
        if (txn_entry_ptr->txnId != processed_txn_id + 1) {
            throw std::runtime_error(fmt::format("Input txnid {}, expected {}", txn_entry_ptr->txnId, processed_txn_id + 1));
        }
        char *buffer_ptr = msg_data_ptr + sizeof(TxnEntry);
        buffers_.push_back(buffer_ptr);
        data_counts_.push_back(txn_entry_ptr->dataCnt);
        txn_entry_.msgCnt++;
        txn_entry_.dataCnt += txn_entry_ptr->dataCnt;
    }
}

/**
 * @brief 从dsp中收取消息，并进行parser
 * 
 * @param pBuffer 
 * @param length 
 * @param pMessageOptr 
 */
void DspMessage::init_msg(const char *pBuffer, int length, std::shared_ptr<ImtMessageOperator> pMessageOptr) {
    //判断pbuffer是否为空，length是否大于0，pMessageOptr是否非空，否则throw error
    if (!pBuffer) {
        throw std::invalid_argument("Buffer pointer is null");
    }
    if (length <= 0) {
        throw std::invalid_argument("Invalid buffer length");
    }
    if (!pMessageOptr) {
        throw std::invalid_argument("Message operator pointer is null");
    }
    if (!pbIntrnlReq_.ParseFromArray(pBuffer, length)) {
        throw std::runtime_error("Parser protobuf message error");
    }
    operator_sp_ = pMessageOptr;
    initialize_ = true;
}

void DspMessage::ack() {
    operator_sp_->acknowledge();
}
#ifndef DSP_MESSAGE_HPP
#define DSP_MESSAGE_HPP

#include "IntrnlMsg.pb.h"
#include <cstdint>
#include <memory>
#include "entry_file.hpp"
#include "txn_entry.hpp"
class ImtMessageOperator {
public:
    void acknowledge() {};
};

class DspMessage {
public:
    static constexpr int block_size = 7576;
    static constexpr int clt_size = sizeof(DatCtrl);
    static constexpr int buffer_size = block_size + clt_size;
    TxnEntry txn_entry_;

private:
    message::intrnl::IntrnlMsg pbIntrnlReq_;
    std::shared_ptr<ImtMessageOperator> operator_sp_;
    int msg_count_;

public:
    bool initialize_;

public:
    DspMessage() : initialize_(false){};
    void init_msg(char *pBuffer, int length, std::shared_ptr<ImtMessageOperator> pMessageOptr);
    void ack();
    void process(int64_t &processed_txn_id);
    void write(std::shared_ptr<EntryFile> index_file, std::shared_ptr<EntryFile> log_file);
};

// void add_repeat_data(message::intrnl::IntrnlMsg &msg, int dataCnt, const TxnEntry &entry);
// message::intrnl::IntrnlMsg createIntrnlMsgs(int totalDataCnt, int txnid, int dataSqno);
std::vector<message::intrnl::IntrnlMsg> create_IntrnlMsg(int totalDataCnt, int64_t &txnid, int dataSqno);
#endif
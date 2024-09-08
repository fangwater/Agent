#ifndef DSP_MESSAGE_HPP
#define DSP_MESSAGE_HPP

#include "IntrnlMsg.pb.h"
#include <cstdint>
#include <vector>
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
    std::vector<char *> buffers_;
    std::vector<int> data_counts_;
private:
    message::intrnl::IntrnlMsg pbIntrnlReq_;
    std::shared_ptr<ImtMessageOperator> operator_sp_;
public:
    bool initialize_;

public:
    DspMessage() : initialize_(false){};
    void init_msg(const char *pBuffer, int length, std::shared_ptr<ImtMessageOperator> pMessageOptr);
    void ack();
    void process(int64_t processed_txn_id);
    void write(std::shared_ptr<EntryFile> index_file, std::shared_ptr<EntryFile> log_file);
};

message::intrnl::IntrnlMsg create_IntrnlMsg_with_repeat(int totalDataCnt, int64_t &txnid, int64_t& dataSqno);
#endif
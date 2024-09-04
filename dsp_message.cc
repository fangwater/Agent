#include "dsp_message.hpp"
#include <fmt/format.h>
#include <vector>
#include "txn_entry.hpp"

// 辅助函数，用于创建 IntrnlMsgData
// void add_repeat_data(message::intrnl::IntrnlMsg &msg, int dataCnt, const TxnEntry &entry) {
//     message::intrnl::IntrnlMsgData *data = msg.add_intrnmsgdata();
//     int64_t msg_len = sizeof(TxnEntry) + dataCnt * DspMessage::buffer_size;
//     std::vector<char> buffer(msg_len);
//     char *buffer_ptr = buffer.data();
//     std::memcpy(buffer_ptr, &entry, sizeof(entry));
//     data->set_data(buffer.data(), buffer.size());
// }

// 主函数，创建 IntrnlMsg
// message::intrnl::IntrnlMsg create_IntrnlMsg_with_repeat(int totalDataCnt, int64_t &txnid, int dataSqno) {
//     txnid++;
//     dataSqno++;
//     message::intrnl::IntrnlMsg msg;
//     constexpr int cnt_max = 3;
//     int remainingDataCnt = totalDataCnt;
//     int currentSqno = dataSqno;
//     TxnEntry entry;
//     entry.txnId = txnid;
//     entry.dataCnt = totalDataCnt;
//     entry.frstDataSqno = dataSqno;
//     entry.lstDataSqno = dataSqno + totalDataCnt - 1;
//     while (remainingDataCnt > 0) {
//         int currentDataCnt = std::min(cnt_max, remainingDataCnt);
//         add_repeat_data(msg, currentDataCnt, entry);
//         // 更新 remainingDataCnt 和 currentSqno
//         remainingDataCnt -= currentDataCnt;
//         currentSqno += currentDataCnt;
//     }
//     dataSqno += totalDataCnt - 1;
//     return msg;
// }
// 将 txn_entry 和相关数据部分添加到消息中，不再是 repeat
void add_data(message::intrnl::IntrnlMsg &msg, int dataCnt, const TxnEntry &entry) {
    message::intrnl::IntrnlMsgData *data = msg.mutable_intrnmsgdata();
    int64_t msg_len = sizeof(TxnEntry) + dataCnt * DspMessage::buffer_size;
    // 构建缓冲区，包含 entry 和数据部分
    std::vector<char> buffer(msg_len);
    char *buffer_ptr = buffer.data();
    // 将 TxnEntry 复制到缓冲区
    std::memcpy(buffer_ptr, &entry, sizeof(entry));
    // 根据 dataCnt 设置数据
    data->set_data(buffer.data(), buffer.size());
}
/**
 * @brief Create a IntrnlMsg object
 * 一条拆分多条
 * @param totalDataCnt 
 * @param txnid 
 * @param dataSqno 
 * @return message::intrnl::IntrnlMsg 
 */
std::vector<message::intrnl::IntrnlMsg> create_IntrnlMsg(int totalDataCnt, int64_t &txnid, int dataSqno) {
    txnid++;
    dataSqno++;
    std::vector<message::intrnl::IntrnlMsg> msg_list;
    constexpr int cnt_max = 3;
    int remainingDataCnt = totalDataCnt;
    int currentSqno = dataSqno;
    TxnEntry entry;
    entry.txnId = txnid;
    entry.dataCnt = totalDataCnt;
    entry.frstDataSqno = dataSqno;
    entry.lstDataSqno = dataSqno + totalDataCnt - 1;
    while (remainingDataCnt > 0) {
        int currentDataCnt = std::min(cnt_max, remainingDataCnt);
        // 每次创建一个新的 IntrnlMsg
        message::intrnl::IntrnlMsg msg;
        // 为每条消息添加数据
        add_data(msg, currentDataCnt, entry);
        // 将消息添加到消息列表
        msg_list.push_back(std::move(msg));
        // 更新 remainingDataCnt 和 currentSqno
        remainingDataCnt -= currentDataCnt;
        currentSqno += currentDataCnt;
    }

    dataSqno += totalDataCnt - 1;

    return msg_list;
}
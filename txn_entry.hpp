#ifndef TXN_ENTRY_HPP
#define TXN_ENTRY_HPP
/**
 * @brief 撮合事务的日志格式
 * 保持内容变量命名和原有一致
 */
#include <cstdint>
#include <string>
#include <fmt/format.h>
struct TxnEntry {
    int64_t txnId;
    int32_t dataCnt;
    int64_t frstDataSqno;
    int64_t lstDataSqno;
    int32_t dataSize;
    int32_t filler;
    int64_t dataOffset;
    int32_t msgSqno;
    int32_t msgCnt;
    int64_t timestamp;
    std::string to_string();
};

/**
 * @brief 事务日志记录中的额外字段
 * 
 */
struct DatCtrl {
    int64_t dataSqno;
    int32_t elemTtlSize;
    int32_t elemCnt;
    int64_t prevDataSqno;
    int64_t nextDataSqno;
    int64_t currMsgSqno;
};

#endif
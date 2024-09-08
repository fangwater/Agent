#include "txn_entry.hpp"
std::string TxnEntry::to_string() {
    return fmt::format("txnId: {}\n"
                       "dataCnt: {}\n"
                       "frstDataSqno: {}\n"
                       "lstDataSqno: {}\n",
                       txnId, dataCnt, frstDataSqno, lstDataSqno);
}

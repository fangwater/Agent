#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include "dsp_message.hpp"
#include "txn_recorder.hpp"
#include "utils.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include "utils.hpp"
enum class HandlerState {
    INIT,
    ALIVE
};

class MessageHandler {
public:
    MessageHandler(bool flag, std::string dir, int id);
    bool get_flag();
    void try_switch(bool target_flag);
    void start_process();
    void stop_process();
    void push_message(std::shared_ptr<DspMessage> dsp_msg);
    int64_t get_processed_txnid() const ;
    int64_t get_processed_total_data_count() const;

public : Logger logger_;
    TxnRecorder txn_recorder_;
    std::atomic_bool reset_requested_;
    std::mutex mutex_;
    std::atomic_bool flag_;
    bool inner_flag_;
    int id_;
    std::atomic_bool running_;
    HandlerState state_;
    std::condition_variable cv_;
    std::condition_variable reset_cv_;
    std::queue<std::shared_ptr<DspMessage>> message_queue_;
};
#endif
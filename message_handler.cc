#include "message_handler.hpp"
#include "dsp_message.hpp"
#include "txn_recorder.hpp"
#include <cmath>
#include <fmt/format.h>
#include <memory>
#include <stdexcept>
#include <sys/types.h>


MessageHandler::MessageHandler(bool flag, std::string dir, int id)
    : state_(HandlerState::INIT), inner_flag_(flag), reset_requested_(false), running_(true), txn_recorder_(dir), id_(id){};

bool MessageHandler::get_flag() {
    return flag_.load();
}

void MessageHandler::try_switch(bool target_flag) {
    flag_ = target_flag;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        reset_requested_ = true;
        logger_->info("agent[{}] reset flag, waiting......", id_);
        reset_cv_.wait(lock, [this] { return reset_requested_ == false; });
        logger_->info("agent[{}] switch completed!", id_);
    }
}

void MessageHandler::start_process() {
    while (running_ || message_queue_.empty()) {
        /*
             * handler分为两种状态，alive和init状态
             *  1、启动时，默认处于init状态
             *  2、连续10s没有收到消息(queue保持为empty超过10s)，并且此时处于init，则可以切换到alive状态
             *  3、alive状态下，可以被设置reset符号，但不立即切换，同样需要保持10s queue为empty的状态，则从alive切换为init
            */
        while (state_ == HandlerState::INIT) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (cv_.wait_for(lock, std::chrono::seconds(10)) == std::cv_status::timeout) {
                //因为超时退出，则切为alive
                logger_->info("agent[{}] no message received in the last 10 seconds, switch to [ALIVE] state...", id_);
                state_ = HandlerState::ALIVE;
                if (reset_requested_) {
                    inner_flag_ = get_flag();
                    if (inner_flag_) {
                        logger_->info("agent[{}] Set the process flag to true", id_);
                    } else {
                        logger_->info("agent[{}] Set the process flag to false", id_);
                    }
                    reset_requested_ = false;
                    reset_cv_.notify_one();
                }
                break;
            } else {
                //因为push message退出，继续保持
                state_ = HandlerState::ALIVE;
            }
        }
        /**
             * @brief 为什么消息处理可以放在第一个部分
             * 1、初始时，ME没有产生消息，!emtpy判定false，直接退出
             * 2、当IMT event的sub被调用，即产生新的主，需要更新flag，此时不会直接修改flag，而是触发request更新 
             */
        {
            std::lock_guard<std::mutex> lk(mutex_);
            while (!message_queue_.empty()) {
                auto dsp_msg = message_queue_.front();
                message_queue_.pop();
                if (!dsp_msg->initialize_) {
                    logger_->info("agent[{}] try process a no initialized msg!", id_);
                    throw std::runtime_error("try process a no initialized msg!");
                }
                dsp_msg->process(txn_recorder_.get_processed_txn_id());
                if (inner_flag_) {
                    txn_recorder_.write_dsp_message(dsp_msg);
                }
                dsp_msg->ack();
                txn_recorder_.append_txn_entry(dsp_msg->txn_entry_);
            }
            /**
                * @brief 若发现此时，reset的标志被设置，则代表ME发生了备切主
                * 原因: 初始时，默认和ME一起的是备，即Agent默认flag为true，备切主，设置为false
                * 收到dsp选主的IMT event后，把flag改为false，此时flag不能立刻生效，而是重置为INIT，flag再生效
                */
            if (reset_requested_) {
                logger_->info("agent[{}] reset requested, switching to [INIT] state...", id_);
                //flag的变化不会立即生效，而是在状态切换之后
                state_ = HandlerState::INIT;
                reset_requested_ = false;
                reset_cv_.notify_one();
            }
        }
    }
}
void MessageHandler::stop_process() {
    running_ = false;
}

void MessageHandler::push_message(std::shared_ptr<DspMessage> dsp_msg) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        message_queue_.push(dsp_msg);
    }
    cv_.notify_one();
}

int64_t MessageHandler::get_processed_txnid() const {
    logger_->info("Agent[{}] was asked txnid {}", id_, txn_recorder_.get_processed_txn_id());
    return txn_recorder_.get_processed_txn_id();
}

int64_t MessageHandler::get_processed_total_data_count() const {
    return txn_recorder_.get_processed_total_data_count();
}

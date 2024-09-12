#include "message_handler.hpp"
#include "dsp_message.hpp"
#include "txn_recorder.hpp"
#include "utils.hpp"
#include <chrono>// 包含chrono库
#include <cmath>
#include <fmt/format.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <sys/types.h>


MessageHandler::MessageHandler(Logger logger, bool flag, std::string dir, int id)
    : logger_(logger), state_(HandlerState::INIT), inner_flag_(flag), reset_requested_(false), running_(true), txn_recorder_(dir,logger), id_(id){};

bool MessageHandler::get_flag() {
    return flag_.load();
}


void MessageHandler::try_switch(bool target_flag) {
    flag_ = target_flag;
    spdlog::info("agent[{}] reset flag, waiting......", id_);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        reset_requested_ = true;
        // 记录开始时间
        auto start = std::chrono::steady_clock::now();
        // 等待条件变量，直到 reset_requested_ 为 false
        reset_cv_.wait(lock, [this] { return reset_requested_ == false; });
        // 记录结束时间
        auto end = std::chrono::steady_clock::now();
        // 计算时间差并转换为秒
        std::chrono::duration<double> elapsed_seconds = end - start;
        spdlog::info("agent[{}] switch completed! took {:.1f} seconds", id_, elapsed_seconds.count());
    }
}


void MessageHandler::start_process() {
    spdlog::info("start agent[{}] message handler!", id_);
    while (running_ || message_queue_.empty()) {
        /*
        * handler分为两种状态，alive和init状态
        *  1、启动时，默认处于init状态
        *  2、连续10s没有收到消息(queue保持为empty超过10s)，并且此时处于init，则可以切换到alive状态
        *  3、alive状态下，可以被设置reset符号，但不立即切换，同样需要保持10s queue为empty的状态，则从alive切换为init
        *  4、切换时候，检查request flag，从flag = true(写)到flag = false(不写)必须在此时进行，其含义为，只有当上一次dsp
        * 消息全部写完，才能进入不写的状态
        *  5、若之不写，现在写，不依赖于request flag，而是依赖于agent的自身状态检查
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
                        logger_->info("agent[{}] set the process flag to true", id_);
                    } else {
                        logger_->info("agent[{}] set the process flag to false", id_);
                    }
                    reset_requested_ = false;
                    reset_cv_.notify_one();
                }
                break;
            }
            else {
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
                /**
                 * @brief ACK的逻辑修改
                 * 原先依赖于flag判断主备关系，flag = true写，flag = false不写
                 * 现逻辑:
                 * 1、flag = true时，直接写
                 * 2、flag = false时，每次对文件进行判断
                 */
                if (!inner_flag_) {
                    if (!txn_recorder_.check_if_txn_already_written(dsp_msg)) {
                        inner_flag_ = true;
                    }
                }

                if(inner_flag_){
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
            /**
             * @brief 此处状态切换存在问题
             * 1、当secondary需要提升为primary的时候，会使用try_switch设置，进入init
             * 2、INIT由于参考首次启动，默认共存的ME为备，因此直接进入secondary
             * 3、此时，如果primary之前仍有剩余消息，由于以及进入secondary状态，所以会被写入，因此存在bug
             * @brief 修改方法
             * 1、首次从init进入secondary，同样需要卡住
             */
            if (reset_requested_) {
                logger_->info("agent[{}] reset requested, switching to [INIT] state...", id_);
                //flag的变化不会立即生效，而是在状态切换之后
                state_ = HandlerState::INIT;
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

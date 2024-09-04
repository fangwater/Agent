#ifndef MATCH_ENGINE_HPP
#define MATCH_ENGINE_HPP
#include "agent.hpp"
#include "dsp_channel.hpp"
#include "dsp_message.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <thread>
#include <vector>

void add_repeat_data(message::intrnl::IntrnlMsg &msg, int dataCnt, const TxnEntry &entry);
message::intrnl::IntrnlMsg create_IntrnlMsg(int totalDataCnt, int64_t &txnid, int64_t &dataSqno);

class MatchEngine {
public:
    int64_t txnid;
    int64_t dataSqno;
    int id_;
    int acked_txnid;
    std::vector<std::shared_ptr<Agent>> agents_;
    std::atomic_bool running_;

public:
    MatchEngine(int id) : id_(id), running_(false){};
    void bind_agents(std::vector<std::shared_ptr<Agent>> &agents) {
        agents_ = agents;
    }
    void stop() {
        running_.store(false);
    }
    void run(std::shared_ptr<DspChannel> channel) {
        //ME启动之前，先向同一台机器上的agent请求状态
        if (agents_[id_]) {
            while (true) {
                if (agents_[id_]->is_txn_ready()) {
                    fmt::println("txn file of agent{} is ready...", id_);
                    break;
                } else {
                    fmt::println("txn file of agent{} is not ready, check agagin...", id_);
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        }
        //数据启动完成，向agent请求最新的Txnid和Total的消息数量
        txnid = agents_[id_]->message_handler_->get_processed_txnid();
        acked_txnid = txnid;
        dataSqno = agents_[id_]->message_handler_->get_processed_total_data_count();
        fmt::println("last txn_id: {}, last data_seq: {}", txnid, dataSqno);
        //请求结束后，开始产生虚假的dsp消息
        running_ = true;
        while (running_) {
            auto msg = create_IntrnlMsg(5, txnid, dataSqno);
            size_t msg_len = msg.ByteSizeLong();
            std::vector<char> msg_data(msg_len);
            if (!msg.SerializeToArray(msg_data.data(), msg_len)) {
                throw std::runtime_error("msg serialize failed!");
            }
            channel->push_message(msg_data);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
};

#endif
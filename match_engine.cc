#include "match_engine.hpp"
#include "dsp_message.hpp"

MatchEngine::MatchEngine(int id) : id_(id), running_(false) {}

void MatchEngine::bind_agents(std::vector<std::shared_ptr<Agent>> &agents) {
    agents_ = agents;
}

void MatchEngine::stop() {
    running_.store(false);
}

void MatchEngine::run(std::shared_ptr<DspChannel> channel) {
    //ME启动之前，先向同一台机器上的agent请求状态
    if (agents_[id_]) {
        while (true) {
            if (agents_[id_]->is_txn_ready()) {
                spdlog::info("txn file of agent{} is ready...", id_);
                break;
            } else {
                spdlog::info("txn file of agent{} is not ready, check agagin...", id_);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    }
    //数据启动完成，向agent请求最新的Txnid和Total的消息数量
    txnid_ = agents_[id_]->get_processed_txnid();
    acked_txnid_ = txnid_;
    dataSqno_ = agents_[id_]->get_processed_total_data_count();
    spdlog::info("last txn_id: {}, last data_seq: {}", txnid_, dataSqno_);
    //请求结束后，开始产生虚假的dsp消息
    running_ = true;
    while (running_) {
        auto msg = create_IntrnlMsg_with_repeat(5, txnid_, dataSqno_);
        size_t msg_len = msg.ByteSizeLong();
        std::vector<char> msg_data(msg_len);
        if (!msg.SerializeToArray(msg_data.data(), msg_len)) {
            spdlog::info("msg serialize failed!");
            throw std::runtime_error("msg serialize failed!");
        }
        //ME push之前，写索引并flush
        
        channel->push_message(msg_data);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
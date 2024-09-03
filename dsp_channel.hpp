#ifndef DSP_CHANNEL_HPP
#define DSP_CHANNEL_HPP
#include "agent.hpp"
#include "tbb/concurrent_queue.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
/**
 * @brief 模拟DSP的功能，可以进行订阅和消息推送
 */
class DspChannel {
public:
    std::mutex mu_;
    std::atomic<bool> running_;
    //TXN通道的消息推送功能，一发多收
    std::unordered_map<int, std::shared_ptr<Agent>> agents_;
    std::string current_master_;
    std::shared_ptr<tbb::concurrent_queue<std::vector<char>>> message_queue_;
    DspChannel() : running_(false) {
        message_queue_ = std::make_shared<tbb::concurrent_queue<std::vector<char>>>();
    };
    //agent的订阅和取消订阅
    void sub(std::shared_ptr<Agent> agent);
    bool unsub(int id);
    void push_message(const std::vector<char> &msg);
    void reset_master(std::string master);
    void start_channel();
    void stop_channel();
};

#endif

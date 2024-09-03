#include "dsp_channel.hpp"
#include <mutex>
#include <unordered_map>
void DspChannel::sub(std::shared_ptr<Agent> agent) {
    std::lock_guard<std::mutex> lk(mu_);
    auto id = agent->get_id();
    auto iter = agents_.find(id);
    if (iter != agents_.end()) {
        iter->second = agent;
    } else {
        agents_[id] = agent;
    }
    agent->handle_subscribe_event(current_master_);
}

bool DspChannel::unsub(int id) {
    std::lock_guard<std::mutex> lk(mu_);
    auto iter = agents_.find(id);
    if (iter != agents_.end()) {
        agents_.erase(iter);
        return true;
    } else {
        return false;
    }
}

/**
 * @brief 收到消息然后输入queue中
 * @param msg 
 */
void DspChannel::push_message(const std::vector<char> &msg) {
    message_queue_->push(msg);
}

void DspChannel::start_channel() {
    running_ = true;
    std::vector<char> v;
    while (!message_queue_->empty() || running_) {
        if (message_queue_->try_pop(v)) {
            //v推送到所有的agent
            {
                std::lock_guard<std::mutex> lk(mu_);
                for (auto iter = agents_.begin(); iter != agents_.end(); iter++) {
                    iter->second->push(v);
                }
            }
        }
    }
}

void DspChannel::stop_channel() {
    running_ = false;
}

void DspChannel::reset_master(std::string master) {
    current_master_ = master;
    for (auto iter = agents_.begin(); iter != agents_.end(); iter++) {
        iter->second->handle_subscribe_event(current_master_);
    }
}
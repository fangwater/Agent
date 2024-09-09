#include "dsp_channel.hpp"
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
bool DspChannel::sub(std::shared_ptr<Agent> agent) {
    if (!master_setted_) {
        throw std::runtime_error("can not sub before master setted!");
    }
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto id = agent->get_id();
        auto iter = agents_.find(id);
        if (iter != agents_.end()) {
            iter->second = agent;
        } else {
            agents_[id] = agent;
        }
    }
    if (agent->get_host_name() == current_master_) {
        return false;
    } else {
        return true;
    }
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
    master_setted_ = true;
    for (auto iter = agents_.begin(); iter != agents_.end(); iter++) {
        std::thread handle_tr([iter, this]() {
            iter->second->handle_subscribe_event(current_master_);
        });
        handle_tr.join();
    }
}
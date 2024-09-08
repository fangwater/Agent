#ifndef AGENT_HPP
#define AGENT_HPP

#include "message_handler.hpp"
#include <atomic>
#include <fmt/format.h>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include "utils.hpp"
/**
 * @brief Agent的状态切换
 * 业务中涉及的状态切换包括:
 * 1 Agent首次启动
   首次启动, mssage_handler默认为备状态, 首次收到imt-event后，判断是否需要切换主备
   若当前Agent为INIT，默认为备，切备则忽略
   若当前Agent为Secondary，切备也忽略
   若当前Agent为Primary，则需要切换
 * 2 Agent在运行过程中重启
   和首次启动相同
 * 3 Agent备切主
 * 4 Agent主切备
 * 
 */
class Agent {
public:
    enum class AgentState {
        PRIMARY,
        SECONDARY,
        TO_SECONDARY,
        TO_PRIMARY,
        INIT
    };

private:
    static std::string state_str(AgentState s);
public:
    Agent() = delete;
    Agent(std::string host_name, std::string dir, int agent_id);
    bool is_txn_ready();
    void handle_subscribe_event(const std::string &master);
    void push(const std::vector<char> &message);
    std::string current_state();
    int get_id() const;
    //moz-kv instance
    std::unique_ptr<MessageHandler> message_handler_;

private:
    /**
     * @brief 记录当前Agent对Txn的数据记录状态
     * 
     */
    std::atomic<AgentState> agent_state_;
    /**2
     * @brief 分为两种情况
     * 1、当role从备切主时，ME需要读取同步等待，确认已经消费完dsp中全部消息(10s无新消息)，额外的，增加一个id对齐
     * 2、当role从主切备，ME无需获取数据。
     */
    std::atomic_bool is_txn_ready_;
    std::mutex mu_;
    int agent_id_;
    std::string host_name_;
    Logger logger_;
    
};

#endif

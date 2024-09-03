#ifndef AGENT_MANAGER_HPP
#define AGENT_MANAGER_HPP

#include "agent.hpp"
#include "dsp_channel.hpp"
#include <future>
#include <memory>
#include <mutex>
#include <vector>

class AgentAccesser {
public:
    AgentAccesser(std::shared_ptr<Agent> agent, std::mutex &mtx)
        : agent_(std::move(agent)), mtx_(mtx), lock_(mtx_, std::adopt_lock) {}

    // 访问借用的Agent
    std::shared_ptr<Agent> get() const {
        return agent_;
    }

private:
    std::shared_ptr<Agent> agent_;
    std::mutex &mtx_;
    std::unique_lock<std::mutex> lock_;
};

class AgentManager {
public:
    std::shared_ptr<Agent> build_agent(int i);
    AgentManager() = delete;
    AgentManager(int num);
    void restart_agent(int idx);

    // 新增的接口方法
    AgentAccesser get_agent(int idx);

private:
    struct AgentData {
        std::shared_ptr<Agent> agent;
        std::future<void> runner;
        std::mutex agent_mutex;
    };

    std::vector<AgentData> agents_data_;
    std::shared_ptr<DspChannel> channel_;
    std::future<void> channel_runner_;
};

#endif// AGENT_MANAGER_HPP

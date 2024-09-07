#ifndef MATCH_ENGINE_HPP
#define MATCH_ENGINE_HPP
#include "agent.hpp"
#include "dsp_channel.hpp"
#include "utils.hpp"
#include <atomic>
#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <vector>

class MatchEngine {
public:
    int64_t txnid_;
    int64_t dataSqno_;
    int id_;
    int acked_txnid_;
    std::vector<std::shared_ptr<Agent>> agents_;
    std::atomic_bool running_;
    Logger logger_;

public:
    MatchEngine(int id);
    void bind_agents(std::vector<std::shared_ptr<Agent>> &agents);
    void stop();
    void run(std::shared_ptr<DspChannel> channel);
};

#endif
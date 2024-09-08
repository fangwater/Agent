#include "test_case.hpp"
#include "agent.hpp"
#include "dsp_message.hpp"
#include "match_engine.hpp"
#include "spdlog/spdlog.h"
#include "txn_entry.hpp"
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

std::shared_ptr<Agent> build_agent(int i) {
    std::string agent_path = fmt::format("../agent{}", i);
    return std::make_shared<Agent>(fmt::format("host{}", i), agent_path, i);
}

void print_file_status(const std::vector<std::shared_ptr<Agent>>& agents) {
    spdlog::info("===================");
    for (int i = 0; i < agents.size(); i++) {
        size_t file_size = std::filesystem::file_size(fmt::format("../agent{}/txn_index.bin", i));
        size_t log_size = std::filesystem::file_size(fmt::format("../agent{}/txn_content.bin", i));
        spdlog::info("Agent[{}]|{:16}|{:16}|", agents[i]->get_id(), file_size/sizeof(TxnEntry), log_size/DspMessage::buffer_size);
    }
    spdlog::info("===================");
}

void print_agent_status(const std::vector<std::shared_ptr<Agent>> &agents) {
    spdlog::info("===================");
    for (int i = 0; i < agents.size(); i++) {
        spdlog::info("Agent[{}]|{:16}|", agents[i]->get_id(), agents[i]->current_state());
    }
    spdlog::info("===================");
}


std::vector<std::shared_ptr<Agent>> build_agents(int num, std::shared_ptr<DspChannel> channel) {
    std::vector<std::shared_ptr<Agent>> agents;
    for (int i = 0; i < num; i++) {
        auto agt = build_agent(i);
        agents.push_back(agt);
        channel->sub(agt);
    }
    return agents;
}

void test_base() {
    auto channel = std::make_shared<DspChannel>();
    auto agents = build_agents(4, channel);
    //此时4个agent均为备，模拟dsp
    std::vector<std::thread> run_agents;
    for (auto &agent: agents) {
        run_agents.emplace_back(std::thread([agent]() {
            agent->message_handler_->start_process();
        }));
    }
    std::thread run_channel([channel]() {
        channel->start_channel();
    });
    //ME开始请求agent，询问是否当前数据可用
    auto match_eng = std::make_shared<MatchEngine>(0);
    match_eng->bind_agents(agents);
    std::thread run_me([match_eng, channel]() {
        match_eng->run(channel);
    });
    //假设ME0为主，ME123为备份
    print_agent_status(agents);
    channel->reset_master("host0");
    //ME 500ms 产生一次消息，持续打印
    for (int k = 0; k < 3; k++) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        print_file_status(agents);
    }
    //模拟出现主备切换的情况，且当dsp中存在消息堵塞
    //1 关闭ME 让ME不再产生消息
    match_eng->stop();
    run_me.join();
    //2 重启ME
    //假设ME1为主，ME34为备份, 0此时已经假设不可用
    match_eng = std::make_shared<MatchEngine>(1);
    match_eng->bind_agents(agents);
    std::thread run_me_1([match_eng, channel]() {
        //重新运行ME，此时ME去跟agent1请求ready，会fail
        match_eng->run(channel);
    });
    //订阅事件产生，Agent1切换状态，txn_ready通过
    channel->reset_master("host1");
    //ME 500ms 产生一次消息，持续打印
    for (int k = 0; k < 3; k++) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        spdlog::info("===================");
        for (int i = 0; i < agents.size(); i++) {
            size_t file_size = std::filesystem::file_size(fmt::format("../agent{}/txn_index.bin", i));
            size_t log_size = std::filesystem::file_size(fmt::format("../agent{}/txn_log_content.bin", i));
            spdlog::info("Agent[{}]|{:16}|{:16}|", agents[i]->get_id(), file_size, log_size);
        }
        spdlog::info("===================");
    }
    for (auto &t: run_agents) {
        t.join();
    }
    run_channel.join();
}

void test_secondary_agent_restart() {
    auto channel = std::make_shared<DspChannel>();
    auto agents = build_agents(2, channel);
    std::vector<std::thread> run_agents;
    for (auto &agent: agents) {
        run_agents.emplace_back(std::thread([agent]() {
            agent->message_handler_->start_process();
        }));
    }
    std::thread run_channel([channel]() {
        channel->start_channel();
    });
    //ME开始请求agent，询问是否当前数据可用
    auto match_eng = std::make_shared<MatchEngine>(0);
    match_eng->bind_agents(agents);
    std::thread run_me([match_eng, channel]() {
        match_eng->run(channel);
    });
    //假设ME0为主，ME123为备份
    channel->reset_master("host0");
    std::thread restart_agent([agents, channel]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        agents[1]->message_handler_->stop_process();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        //Agent重启默认为备，
        channel->sub(agents[1]);
        agents[1]->message_handler_->start_process();
    });
    //ME 500ms 产生一次消息，持续打印
    for (int k = 0; k < 3; k++) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    // match_eng->stop();
    run_me.join();
    restart_agent.join();
    for (auto &t: run_agents) {
        t.join();
    }
    run_channel.join();
}


void test_primary_agent_restart() {
    auto channel = std::make_shared<DspChannel>();
    auto agents = build_agents(2, channel);
    std::vector<std::thread> run_agents;
    for (auto &agent: agents) {
        run_agents.emplace_back(std::thread([agent]() {
            agent->message_handler_->start_process();
        }));
    }
    std::thread run_channel([channel]() {
        channel->start_channel();
    });
    //ME开始请求agent，询问是否当前数据可用
    auto match_eng = std::make_shared<MatchEngine>(0);
    match_eng->bind_agents(agents);
    std::thread run_me([match_eng, channel]() {
        match_eng->run(channel);
    });
    //假设ME0为主，ME123为备份
    channel->reset_master("host0");
    std::thread restart_agent([agents, channel]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        agents[0]->message_handler_->stop_process();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        //Agent重启默认为备，
        channel->sub(agents[1]);
        agents[0]->message_handler_->start_process();
    });
    //ME 500ms 产生一次消息，持续打印
    for (int k = 0; k < 3; k++) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        print_file_status(agents);
    }
    // match_eng->stop();
    run_me.join();
    restart_agent.join();
    for (auto &t: run_agents) {
        t.join();
    }
    run_channel.join();
}

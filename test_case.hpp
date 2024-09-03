#ifndef TEST_CASE_HPP
#define TEST_CASE_HPP
#include "agent.hpp"
#include "dsp_channel.hpp"
#include "fmt/core.h"
#include "match_engine.hpp"
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

std::shared_ptr<Agent> build_agent(int i);
std::vector<std::shared_ptr<Agent>> build_agents(int num, std::shared_ptr<DspChannel> channel);
void test_base();
void test_primary_agent_restart();
void test_secondary_agent_restart();
#endif

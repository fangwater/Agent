#ifndef TEST_CASE_HPP
#define TEST_CASE_HPP
#include "agent.hpp"
#include "dsp_channel.hpp"
#include <memory>
#include <vector>
std::vector<std::shared_ptr<Agent>> build_agents(int num, std::shared_ptr<DspChannel> channel);
void test_base();
void test_primary_agent_restart();
void test_secondary_agent_restart();
#endif

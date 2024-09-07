#include "agent.hpp"
#include "dsp_message.hpp"
#include "message_handler.hpp"
#include "utils.hpp"
#include <memory>
#include <stdexcept>
#include <sys/types.h>

std::string Agent::state_str(AgentState s) {
    switch (s) {
        case AgentState::INIT:
            return "INIT";
        case AgentState::PRIMARY:
            return "PRIMARY";
        case AgentState::TO_PRIMARY:
            return "TO_PRIMARY";
        case AgentState::TO_SECONDARY:
            return "TO_SECONDARY";
        case AgentState::SECONDARY:
            return "SECONDARY";
        default:
            break;
    }
    return "NULL";
}
Agent::Agent(std::string host_name, std::string dir, int agent_id)
    : host_name_(host_name), agent_id_(agent_id), agent_state_(AgentState::INIT){
    logger_ = create_logger(fmt::format("agent[{}]", agent_id_));
    //Agent默认ME为备，flag = true标志位需要写的状态
    message_handler_ = std::make_unique<MessageHandler>(logger_,true, dir, agent_id);
}


/**
 * @brief 仅用于当Agent启动，或者发生主备切换以后，ME(主)向Agent询问，当前的Txn数据是完整状态
 * 判断依据
 * 1、Agent启动进入INIT状态, 进入Primary的才会成为候选
 * 2、当发生主备切换时，新的ME被提升为主，作为Secondary的Agent需要接收全部信息，即成功切主之后，才可以访问
 * 3、接口仅查询当前是否为Primary，不保证是Secondary切换而来
 * 4、若切换为Secondary之后，ME有新消息产生，同样不保证txn的完整性
 * @return true 
 * @return false 
 */
bool Agent::is_txn_ready() {
    if (agent_state_ == AgentState::PRIMARY) {
        logger_->info("reply for txn-ready check: agent is in primary, txn file is ready to use.");
        return true;
    } else if (agent_state_ == AgentState::TO_PRIMARY) {
        logger_->info("reply for txn-ready check: agent in switching state....");
        return false;
    } else if (agent_state_ == AgentState::SECONDARY){
        logger_->info("reply for txn-ready check: agent has not started state switching yet.");
        return false;
    } else {
        logger_->error("current agent state is: [{}], should not check txn ready!",state_str(agent_state_));
        return false;
    }
}


/**
 * @brief 响应ME主备切换事件(ME主变更)，或Agent重启后(查询当前主ME，判定自身状态)
 * 1、agent的host地址和主ME一致，切换为PRIMARY状态
 * @param master 当前为主的ME名称
 */
void Agent::handle_subscribe_event(const std::string &master) {
    if (host_name_ == master) {
        if (agent_state_ == AgentState::INIT) {
            //初始为INIT状态，切换为PRIMARRY
            agent_state_ = AgentState::TO_PRIMARY;
            logger_->info("agent state change due to init subscribe:");
            logger_->info("current state: []...", state_str(agent_state_));
            message_handler_->try_switch(false);
            agent_state_ = AgentState::PRIMARY;
            logger_->info("success switch to []!", state_str(agent_state_));
        } else if(agent_state_ == AgentState::PRIMARY){
            /**
             * @brief 已经为主，然后收到订阅事件，且切换目标为主
             * 1、primary agent正在运行, 主ME重启，DSP控制此种情况不会发生(ME不会原地重启，而是进行主备切换)
             * 2、主ME运行，primary agent重启，此时只会从INIT 切换
             * 结论: 如果存在agent 的 primary 切 primary 则为逻辑错误
             */
            throw std::runtime_error("agent state error due to primary to primary switch!");
        } else if (agent_state_ == AgentState::SECONDARY) {
            /**
             * @brief Agent为当前为secondary，收到订阅事件需要切主
             * ME先前为备，因此agent为secondary，然后被选为主，此时agent跟随切主
             */
            logger_->info("agent state change due to master match_engine re-select!");
            logger_->info("current state: []...", state_str(agent_state_));
            message_handler_->try_switch(false);
            agent_state_ = AgentState::PRIMARY;
            logger_->info("success switch to []!", state_str(agent_state_));
        } else {
            logger_->error("error for agent in {} and try switch {}",state_str(agent_state_),state_str(AgentState::PRIMARY));
            throw std::runtime_error("error state");
        }
    } else {
        //host 和 master不相同，当前Agent归属状态为primary
        if (agent_state_ == AgentState::INIT) {
            //初始为INIT状态 message_handler默认写，无需切换
            logger_->info("agent state change due to init subscribe:");
            logger_->info("current state: []...", state_str(agent_state_));
            agent_state_ = AgentState::SECONDARY;
            logger_->info("success switch to []!", state_str(agent_state_));
        } else if(agent_state_ == AgentState::SECONDARY){
            //Agent重启，但保持secondary的位置
            logger_->info("primary agent restart, keep state...");
        } else if (agent_state_ == AgentState::PRIMARY) {
            /**
             * @brief Agent原先为primary，ME重选被降为备或挂掉，需要降级为备份
             * (或者弃用？禁止重启)
             */
            logger_->info("agent state change due to master match_engine re-select!");
            logger_->info("current state: []...", state_str(agent_state_));
            agent_state_ = AgentState::TO_SECONDARY;
            message_handler_->try_switch(true);
            agent_state_ = AgentState::SECONDARY;
            logger_->info("success switch to []!", state_str(agent_state_));
        } else {
            logger_->error("error for agent in {} and try switch {}", state_str(agent_state_), state_str(AgentState::SECONDARY));
            throw std::runtime_error("error state");
        }
    }
}

/**
 * @brief Agent收到dsp推送的消息字符流，填充pb消息，然后返回
 * 
 */
void Agent::push(const std::vector<char> &message) {
    std::shared_ptr<DspMessage> msg_ptr = std::make_shared<DspMessage>();
    auto ack_ptr = std::make_shared<ImtMessageOperator>();
    msg_ptr->init_msg(message.data(), message.size(), ack_ptr);
    message_handler_->push_message(msg_ptr);
    return;
}

int Agent::get_id() const {
    return agent_id_;
}
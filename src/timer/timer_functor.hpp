#pragma once

#include <message/message.hpp>
#include <timer/timer_message.hpp>
#include <bus/message_bus.hpp>

namespace micro
{
    namespace core
    {

        inline void broadcast_tick_notification(uint64_t time_tick)
        {
            std::shared_ptr<message> msg = std::make_shared<message>();
            msg->set_name(BROADCAST_TIMER_TICK);
            
            std::shared_ptr<broadcast_timer_tick> msg_body = std::make_shared<broadcast_timer_tick>();
            msg_body->time_tick = time_tick;
            msg->m_body = msg_body;

            MSG_BUS_PUB<int32_t, std::shared_ptr<message>>(BROADCAST_TIMER_TICK, msg);
        }

    }

}

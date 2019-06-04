#pragma once

#include <message/message.hpp>

#define BROADCAST_TIMER_TICK                                     "broadcast_timer_tick"

namespace micro
{
    namespace core
    {

        class broadcast_timer_tick : public base_body
        {
        public:

            uint64_t time_tick;

        };

    }

}
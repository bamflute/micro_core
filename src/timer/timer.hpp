#pragma once

#include <timer/base_timer.hpp>

namespace micro
{
    namespace core
    {

        class timer : public base_timer
        {
        public:

            timer(const std::string &timer_name, uint64_t period, uint64_t trigger_times, const std::string &info)
                : base_timer(timer_name, period, trigger_times)
                , m_info(info)
            {}

            uint64_t get_first_time_out_tick()
            {

            }

        protected:

            std::string m_info;

        };

    }

}

#pragma once

#include <timer/base_timer.hpp>
#include <timer/timer_generator.hpp>

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
            {
                m_time_out_tick = get_first_time_out_tick();
            }

            uint64_t get_first_time_out_tick()
            {
                return TIMER_GENERATOR.get_tick() + m_period_as_tick;
            }

            const std::string & get_info() const { return m_info; }

        protected:

            std::string m_info;

        };

    }

}

/*********************************************************************************
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
*  date: 2017.10.01
*  author:  Bruce Feng
**********************************************************************************/
#pragma once

#include <timer/timer_common.hpp>

namespace micro
{
    namespace core
    {

        class base_timer
        {
        public:

            base_timer(const std::string &timer_name, uint64_t period, uint64_t trigger_times)
                : m_timer_name(timer_name)
                , m_period(period)
                , m_trigger_times(trigger_times)
                , m_timer_id(0)
            {
                m_period_as_tick = std::lround(m_period * 1.0 / DEFAULT_MILLISECONDS_ONE_TICK);
            }

            virtual ~base_timer() {}

            const std::string &get_name() const { return m_timer_name; }

            uint64_t get_timer_id() const { return m_timer_id; }

            void set_timer_id(uint64_t timer_id) { m_timer_id = timer_id; }

            virtual uint64_t get_first_time_out_tick() { return 0; }

            uint64_t get_time_out_tick() const { return m_time_out_tick; }

            void cal_time_out_tick() { m_time_out_tick += m_period_as_tick; }

            void minus_trigger_times() { if (m_trigger_times > 0) m_trigger_times--; }

            uint64_t get_trigger_times() const { return m_trigger_times; }

        protected:

            std::string m_timer_name;

            uint64_t m_period;  //ms

            uint64_t m_trigger_times;

            uint64_t m_timer_id;

            uint64_t m_time_out_tick;

            uint64_t m_period_as_tick;

        };

    }

}

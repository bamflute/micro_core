#pragma once

#include <module/base_module.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <thread/nio_thread_pool.hpp>
#include <timer/timer_common.hpp>
#include <common/common.hpp>
#include <timer/timer_functor.hpp>

#define TIMER_GENERATOR boost::serialization::singleton<micro::core::timer_generator>::get_mutable_instance()

namespace micro
{
    namespace core
    {

        class timer_generator : public base_module
        {
        public:

            typedef std::shared_ptr<nio_thread_pool> pool_ptr_type;
            typedef std::atomic<std::uint64_t> tick_type;
            typedef boost::asio::steady_timer timer_type;
            typedef std::shared_ptr<timer_type> timer_ptr_type;

            timer_generator() : m_thr_pool(std::make_shared<nio_thread_pool>()), m_timer_tick(0), m_expired_functor(broadcast_tick_notification){}

            ~timer_generator() = default;

            std::string module_name() const { return "timer generator"; }

            int32_t init(any_map &vars)
            {
                return m_thr_pool->init(1);
            }

            int32_t start()
            {
                m_timer = std::make_shared<timer_type>(*m_thr_pool->get_ios());
                if (ERR_SUCCESS != m_thr_pool->start())
                {
                    return ERR_FAILED;
                }

                return start_timer();
            }

            int32_t  stop()
            {
                stop_timer();
                return m_thr_pool->stop();
            }

            int32_t start_timer()
            {
                m_timer->expires_from_now(std::chrono::milliseconds(DEFAULT_MILLISECONDS_ONE_TICK));
                m_timer->async_wait(boost::bind(&timer_generator::on_expired, this, boost::asio::placeholders::error));
                return ERR_SUCCESS;
            }

            void on_expired(const boost::system::error_code& error)
            {
                if (boost::asio::error::operation_aborted == error)
                {
                    return;
                }

                ++m_timer_tick;

                m_expired_functor(m_timer_tick);

                start_timer();
            }

            void stop_timer()
            {
                boost::system::error_code error;
                m_timer->cancel(error);
            }

            int32_t  exit() { return m_thr_pool->exit(); }

            uint64_t get_tick() { return m_timer_tick.load(); }

        protected:

            timer_ptr_type m_timer;

            volatile automic_uint64_type m_timer_tick;

            pool_ptr_type m_thr_pool;

            functor_type m_expired_functor;

        };

    }

}